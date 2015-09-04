# Author: Daniel Ortiz Mart\'inez
# *- bash -*

# Target function example to be used with the downhill package.  The
# target function adjusted by this script is the BLEU translation
# quality measure for the translations generated by a stack decoder.
# The translations are generated using the "thot_decoder" tool,
# which is provided by the "thot" package.

# NOTE: The BLEU measure can be replaced by the WER measure by setting
# the appropriate value of the MEASURE value.

########
calc_nnc_pen()
{
    we="$1"
    nnc="$2"
    pen_fact=$3
    echo "$we" | $AWK -v nnc="${nnc}" -v pen_fact=${pen_fact}\
                      'BEGIN{
                             result=0;
                             split(nnc,nnc_arr," ")
                            }
                            {   
                             for(i=1;i<=NF;++i)
                             {
                              if($i<0 && nnc_arr[i]==1) 
                               result+=$i*pen_fact*(-1)
                             }
                            }
                         END{
                             printf"%f",result
                            }'
}

########
decoder_needs_to_be_run()
{
    if [ ${USE_NBEST_OPT} -eq 0 ]; then
        echo 1
    else
        # Check condition in file
        cond=`cat $SDIR/run_decoder.txt`
        echo $cond
    fi
}

########
diff_is_below_threshold()
{
    nvalue=$1
    perc=$2
    ndiff=$3
    echo "" | $AWK -v ndiff=$ndiff -v nvalue=$nvalue -v perc=$perc '{if(ndiff<=nvalue*perc) printf"1\n"; else printf"0\n"}'
}

########
reg_dec_invok()
{
    file=$1
    if [ -f $file ]; then
        n=`cat $file`
        echo `expr $n + 1` > $file
    else
        echo 1 > $file
    fi
}

########
check_if_files_differ()
{
    # Read parameters
    prevfile=$1
    mergedfile=$2

    # Check differences
    num_lines1=`wc -l ${prevfile} | ${AWK} '{printf"%s\n",$1}'`
    num_lines2=`wc -l ${mergedfile} | ${AWK} '{printf"%s\n",$1}'`
    ndiff=`echo "" | $AWK -v nlp=$num_lines1 -v nlm=$num_lines2 '{printf"%d\n",nlm-nlp}'`
    diff_below_threshold=`diff_is_below_threshold ${OPT_NVALUE} 0.01 $ndiff`
    if [ ${diff_below_threshold} -eq 1 ]; then
        echo 0
    else
        echo 1
        echo "${prevfile} $ndiff" >> $SDIR/nbl_diff.txt
    fi
}

########
execute_decoder()
{
    # Check if pbs version of the decoder is to be executed
    decbase=`${BASENAME} ${PHRDECODER}`
    if [ $decbase = "thot_decoder" ]; then
        pbsdec="yes"
    else
        pbsdec="no"
    fi

    # Appropriately execute decoder
    if [ $pbsdec = "yes" ]; then
        ${PHRDECODER} -c $CFGFILE -t ${TEST} -tmw $weights -sdir ${SDIR} \
            ${qs_opt} "${QS}" ${ADD_DEC_OPTIONS} -v -o ${SDIR}/smt_trgf_aux.trans || decoder_error="yes"
    else
        ${PHRDECODER} -c $CFGFILE -t ${TEST} -tmw $weights \
            ${ADD_DEC_OPTIONS} -v -o ${SDIR}/smt_trgf_aux.trans \
            2> ${SDIR}/smt_trgf_aux.trans.log || decoder_error="yes"
    fi

    # Sanity check (verify if translations were generated)
    local num_trans=`wc -l ${SDIR}/smt_trgf_aux.trans | $AWK '{printf "%s",$1}'`
    local num_trans_test=`wc -l ${TEST} | $AWK '{printf "%s",$1}'`
    if [ ${num_trans} -ne ${num_trans_test} ]; then
        decoder_error="yes"
    fi

    # Treat decoder error if necessary
    if [ "${decoder_error}" = "yes" ]; then
        echo "Error while executing decoder, for additional information see file ${SDIR}/smt_trgf.log" >&2
        exit 1
    fi
}

########
gen_trans()
{
    run_decoder=`decoder_needs_to_be_run`
    if [ ${run_decoder} -eq 1 ]; then
        # Register number of decoder invokations
        reg_dec_invok $SDIR/num_dec_invok.txt

        # Evaluate target function by running the decoder
        execute_decoder
        
        # If n-best optimization is enabled, obtain n-best lists from
        # word-graphs and merge with the previously generated ones
        if [ ${USE_NBEST_OPT} -eq 1 ]; then
            # Delete file with n-best list files that have changed
            if [ -f $SDIR/nbl_diff.txt ]; then
                rm $SDIR/nbl_diff.txt
            fi
            # For each sentence to be translated...
            new_opts_added=0
            for wgfile in `$FIND $nbdir/ -name sentence*.wg | $SORT`; do
                # Generate new n-best list
${bindir}/thot_wg_proc -w $wgfile -n ${OPT_NVALUE} -o $SDIR/process_wg_output 2> ${SDIR}/smt_trgf_proccess_wg.log
                if [ -f $wgfile.nbl ]; then
                    # Merge with previous n-best list file
${bindir}/thot_merge_nbest_list $SDIR/process_wg_output.nbl $wgfile.nbl > $wgfile.merged_nbl

                    # Check differences between the previously generated n-best list and the merged file
                    files_different=`check_if_files_differ $wgfile.nbl $wgfile.merged_nbl`
                    
                    # Rename merged file
                    mv $wgfile.merged_nbl $wgfile.nbl

                    # Check if no new options have been added
                    if [ ${files_different} -eq 1 ]; then
                        new_opts_added=1
                    fi
                else
                    # Copy initial n-best list file
                    cp $SDIR/process_wg_output.nbl $wgfile.nbl
                    new_opts_added=1
                fi
            done
            if [ $new_opts_added -eq 0 ]; then
                echo 0 > $SDIR/run_decoder.txt
            fi
        fi
    fi

    # If USE_NBEST_OPT enabled, obtain translations from n-best list
    if [ ${USE_NBEST_OPT} -eq 1 ]; then
        # Evaluate target function by processing wordgraphs

        # Delete file with translations
        if [ -f ${SDIR}/smt_trgf_aux.trans ]; then
            rm ${SDIR}/smt_trgf_aux.trans
        fi 
        # Process n-best list file for each sentence
        for wgfile in `$FIND $nbdir/ -name sentence*.wg | $SORT`; do
            # Obtain best translation by rescoring the n-best list
${bindir}/thot_obtain_best_trans_from_nbl $wgfile.nbl "$weights" >> ${SDIR}/smt_trgf_aux.trans
        done
    fi
}

########
get_sp_value_from_cfg()
{
    echo `$GREP "\-sp" $CFGFILE | $AWK '{printf"%s",$2}'`
}

########
posproc_output()
{
    SP=`get_sp_value_from_cfg`
    if [ $SP -ne 0 ]; then
        mv ${SDIR}/smt_trgf_aux.trans ${SDIR}/smt_trgf.unpreproc_trans
        mv ${SDIR}/smt_trgf_aux.trans.log ${SDIR}/smt_trgf.unpreproc_trans.log

        # Check if -p option has to be provided
        if [ $SP -ne 3 ]; then
            P_OPT="-p ${RAW_TEST}"
        fi

        # Check if -l option has been provided
        if [ "${PREPROC_FILE}" != "_none_" ]; then
            L_OPT="-l ${PREPROC_FILE}"
        fi

        # Posprocess output
${bindir}/posproc_file -f ${SDIR}/smt_trgf.unpreproc_trans -t $SP ${P_OPT} ${L_OPT} > ${SDIR}/smt_trgf.trans 2> ${SDIR}/posproc.log

        # Set file with references for evaluation purposes
        REF_FOR_EVAL=${RAW_REF}
    else
        mv ${SDIR}/smt_trgf_aux.trans ${SDIR}/smt_trgf.trans
        mv ${SDIR}/smt_trgf_aux.trans.log ${SDIR}/smt_trgf.trans.log

        # Set file with references for evaluation purposes
        REF_FOR_EVAL=${REF}
    fi
}

########
evaluate()
{
    # Use variable MEASURE to switch between different translation quality/error measures
    case $MEASURE in
        "BLEU") # Calculate the BLEU measure
            ${bindir}/thot_calc_bleu -r ${REF_FOR_EVAL} -t  ${SDIR}/smt_trgf.trans >> ${SDIR}/smt_trgf.${MEASURE}
            # Obtain BLEU
            BLEU=`tail -1 ${SDIR}/smt_trgf.${MEASURE} | ${AWK} '{printf"%f\n",1-$2}'`
            # Print target function value
            echo "${BLEU} ${nnc_pen}" | $AWK '{printf"%f\n",$1+$2}'
            ;;
        "WER") # Calculate the WER measure
            ${bindir}/thot_calc_wer ${REF_FOR_EVAL} ${SDIR}/smt_trgf.trans | head -1 >> ${SDIR}/smt_trgf.${MEASURE}
            # Obtain WER
            WER=`tail -1 ${SDIR}/smt_trgf.${MEASURE} | ${AWK} '{printf"%f\n",$3}'`
            # Print target function value
            echo "${WER} ${nnc_pen}" | $AWK '{printf"%f\n",$1+$2}'
            ;;
    esac
}

########

if [ $# -lt 2 ]; then
    echo "Usage: downhill_trgfunc_smt <sdir> <w1> ... <wn>"
else
    # Initialize variables
    if [ "${PHRDECODER}" = "" ]; then PHRDECODER=${bindir}/thot_decoder; fi
    if [ "${CFGFILE}" = "" ]; then CFGFILE="server.cfg" ; fi
    if [ "${BASEDIR}" = "" ]; then BASEDIR=${HOME}/traduccion/corpus/Xerox/en_es/v14may2003/simplified2 ; fi
    if [ "${TEST}" = "" ]; then TEST=${BASEDIR}/DATA/Es-dev ; fi
    if [ "${REF}" = "" ]; then REF=${BASEDIR}/DATA/En-dev ; fi
    if [ "${RAW_TEST}" = "" ]; then RAW_TEST="_none_" ; fi
    if [ "${RAW_REF}" = "" ]; then RAW_REF="_none_" ; fi
    if [ "${PREPROC_FILE}" = "" ]; then PREPROC_FILE="_none_" ; fi
    if [ "${MEASURE}" = "" ]; then MEASURE="BLEU" ; fi
    if [ "${NNC_PEN_FACTOR}" = "" ]; then NNC_PEN_FACTOR=1000; fi
    if [ "${USE_NBEST_OPT}" = "" ]; then USE_NBEST_OPT=0; fi
    if [ "${OPT_NVALUE}" = "" ]; then OPT_NVALUE=200; fi
    if [ "${QS}" != "" ]; then qs_opt="-qs"; fi

    # Check variables
    if [ ! -f ${PHRDECODER} ]; then
        echo "ERROR: file ${PHRDECODER} does not exist" >&2
        exit 1
    fi

    if [ ! -f ${CFGFILE} ]; then
        echo "ERROR: file ${CFGFILE} does not exist" >&2
        exit 1
    fi

    ls ${TM}* >/dev/null 2>&1 || ( echo "ERROR: invalid prefix ${TM}" >&2 ; exit 1 )

    if [ ! -f ${LM} ]; then
        echo "ERROR: file ${LM} does not exist" >&2
        exit 1
    fi

    if [ ! -f ${TEST} ]; then
        echo "ERROR: file ${TEST} does not exist" >&2
        exit 1
    fi

    if [ ! -f ${REF} ]; then
        echo "ERROR: file ${REF} does not exist" >&2
        exit 1
    fi

    # Read parameters
    SDIR=$1
    shift
    NUMW=$#
    weights=""
    while [ $# -gt 0 ]; do
        # Build weight vector
        weights="${weights} $1"
        shift
    done

    # Prepare all the necessary things to process USE_NBEST_OPT variable
    if [ "${USE_NBEST_OPT}" -gt 0 ]; then
        if [ ! -z "${WG_OPT}" ]; then
            echo "WARNING: Since USE_NBEST_OPT is greater than zero, WG parameter is discarded" >&2
        fi
        # Define WG_OPT variable
        nbdir=$SDIR/nbl
        if [ ! -d $nbdir ]; then mkdir $nbdir; fi
        WG_OPT="-wg $nbdir/sentence"
        # Initialize file containing run_decoder condition
        if [ ! -f $SDIR/run_decoder.txt ]; then 
            echo 1 > $SDIR/run_decoder.txt
        fi
    fi

    # Obtain non-negativity constraints penalty (non-negativity
    # constraints can be activated for each individual weight by means
    # of the environment variable NON_NEG_CONST, which contains a bit
    # vector; a value of 1 for i'th vector means that the i'th weight
    # must be positive)
    nnc_pen=0
    if [ ! "${NON_NEG_CONST}" = "" ]; then
        nnc_pen=`calc_nnc_pen "${weights}" "${NON_NEG_CONST}" ${NNC_PEN_FACTOR}`
    fi

    # Print translator config
    ${PHRDECODER} --config > ${SDIR}/trans.cfg 2>&1
    
    # Generate translations
    gen_trans

    # Post-process output if required
    posproc_output

    # Evaluate results
    evaluate
fi
