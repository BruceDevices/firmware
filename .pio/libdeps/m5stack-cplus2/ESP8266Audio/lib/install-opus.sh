#!/bin/bash

rm -rf libopus
mkdir libopus libopus/include libopus/src libopus/celt libopus/silk libopus/silk/fixed
cp opus/include/*.h libopus/include/.
cp opus.config.h libopus/include/config.h

celt="arch.h bands.c bands.h celt.c celt_decoder.c celt.h celt_lpc.c celt_lpc.h cpu_support.h cwrs.c cwrs.h ecintrin.h entcode.c entcode.h entdec.c entdec.h entenc.c entenc.h fixed_debug.h fixed_generic.h float_cast.h kiss_fft.c _kiss_fft_guts.h kiss_fft.h laplace.c laplace.h mathops.c mathops.h mdct.c mdct.h mfrngcod.h modes.c modes.h os_support.h pitch.c pitch.h quant_bands.c quant_bands.h rate.c rate.h stack_alloc.h static_modes_fixed.h static_modes_float.h vq.c vq.h"
cd opus/celt
for i in $celt; do
    cp -i $i ../../libopus/celt/.
done
incs="config.h opus_custom.h opus_defines.h opus.h opus_multistream.h opus_projection.h opus_types.h"
for i in $incs; do
    echo '#include "../include/'$i'"' > ../../libopus/celt/$i
done
sed -i s/HAVE_CONFIG_H/__STDC__/g ../../libopus/celt/*.[ch]

cd ../silk
silk="A2NLSF.c ana_filt_bank_1.c API.h biquad_alt.c bwexpander_32.c bwexpander.c check_control_input.c CNG.c code_signs.c control_audio_bandwidth.c control_codec.c control.h control_SNR.c debug.c debug.h dec_API.c decode_core.c decode_frame.c decode_indices.c decode_parameters.c decode_pitch.c decode_pulses.c decoder_set_fs.c define.h encode_indices.c encode_pulses.c errors.h gain_quant.c HP_variable_cutoff.c init_decoder.c Inlines.h inner_prod_aligned.c interpolate.c lin2log.c log2lin.c LPC_analysis_filter.c LPC_fit.c LPC_inv_pred_gain.c LP_variable_cutoff.c MacroCount.h MacroDebug.h macros.h main.h NLSF2A.c NLSF_decode.c NLSF_del_dec_quant.c NLSF_encode.c NLSF_stabilize.c NLSF_unpack.c NLSF_VQ.c NLSF_VQ_weights_laroia.c NSQ.h pitch_est_defines.h pitch_est_tables.c PLC.c PLC.h process_NLSFs.c quant_LTP_gains.c resampler.c resampler_down2_3.c resampler_down2.c resampler_private_AR2.c resampler_private_down_FIR.c resampler_private.h resampler_private_IIR_FIR.c resampler_private_up2_HQ.c resampler_rom.c resampler_rom.h resampler_structs.h shell_coder.c sigm_Q15.c SigProc_FIX.h sort.c stereo_decode_pred.c stereo_encode_pred.c stereo_find_predictor.c stereo_LR_to_MS.c stereo_MS_to_LR.c stereo_quant_pred.c structs.h sum_sqr_shift.c table_LSF_cos.c tables_gain.c tables.h tables_LTP.c tables_NLSF_CB_NB_MB.c tables_NLSF_CB_WB.c tables_other.c tables_pitch_lag.c tables_pulses_per_block.c tuning_parameters.h typedef.h VQ_WMat_EC.c"
for i in $silk; do
    cp -i $i ../../libopus/silk/.
done
for i in $incs; do
    echo '#include "../include/'$i'"' > ../../libopus/silk/$i
done
silkcelt="arch.h celt_lpc.h cpu_support.h ecintrin.h entdec.h entenc.h os_support.h stack_alloc.h"
for i in $silkcelt; do
    echo '#include "../celt/'$i'"' > ../../libopus/silk/$i
done
silkfix="main_FIX.h structs_FIX.h"
for i in $silkfix; do
    echo '#include "fixed/'$i'"' > ../../libopus/silk/$i
done
sed -i s/HAVE_CONFIG_H/__STDC__/g ../../libopus/silk/*.[ch]

cd fixed
fixed="apply_sine_window_FIX.c autocorr_FIX.c burg_modified_FIX.c corrMatrix_FIX.c find_LPC_FIX.c find_LTP_FIX.c find_pred_coefs_FIX.c k2a_FIX.c k2a_Q16_FIX.c LTP_analysis_filter_FIX.c LTP_scale_ctrl_FIX.c main_FIX.h process_gains_FIX.c regularize_correlations_FIX.c residual_energy16_FIX.c residual_energy_FIX.c schur64_FIX.c schur_FIX.c structs_FIX.h vector_ops_FIX.c warped_autocorrelation_FIX.c"
for i in $fixed; do
    cp -i $i ../../../libopus/silk/fixed/$i
done
incs="config.h"
for i in $incs; do
    echo '#include "../../include/'$i'"' > ../../../libopus/silk/fixed/$i
done
fixedcelt="celt_lpc.h entdec.h entenc.h pitch.h stack_alloc.h"
for i in $fixedcelt; do
    echo '#include "../../celt/'$i'"' > ../../../libopus/silk/fixed/$i
done
fixedsilk="control.h debug.h define.h main.h pitch_est_defines.h PLC.h SigProc_FIX.h structs.h tuning_parameters.h typedef.h"
for i in $fixedsilk; do
    echo '#include "../'$i'"' > ../../../libopus/silk/fixed/$i
done
sed -i s/HAVE_CONFIG_H/__STDC__/g ../../../libopus/silk/fixed/*.[ch]

cd ../../src
src="opus.c opus_decoder.c opus_private.h"
for i in $src; do
    cp -i $i ../../libopus/src/$i
done
incs="config.h opus_custom.h opus_defines.h opus.h opus_multistream.h opus_projection.h opus_types.h"
for i in $incs; do
    echo '#include "../include/'$i'"' > ../../libopus/src/$i
done
srccelt="arch.h celt.h cpu_support.h entdec.h float_cast.h mathops.h modes.h os_support.h stack_alloc.h"
for i in $srccelt; do
    echo '#include "../celt/'$i'"' > ../../libopus/src/$i
done
srcsilk="API.h define.h structs.h"
for i in $srcsilk; do
    echo '#include "../silk/'$i'"' > ../../libopus/src/$i
done
sed -i s/HAVE_CONFIG_H/__STDC__/g ../../libopus/src/*.[ch]

cd ../
root="AUTHORS COPYING LICENSE_PLEASE_READ.txt README"
for i in $root; do
    cp -i $i ../libopus/.
done

cd ..
rm -r ../src/libopus
mv libopus ../src/libopus
