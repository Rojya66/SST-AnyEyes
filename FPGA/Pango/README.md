# Pango source snapshot

These projects preserve the local 501, 601, 701 and UNIT FanBalance design inputs at the DSP baseline. The snapshot uses dual_led.v in place of the Windows reserved filename aux.v. Only the project-relative references changed.

The public snapshot keeps the Pango IP configuration files (.idf). Regenerate the following vendor RTL using the Pango toolchain and the included configurations, or restore it from the licensed local project at the matching path before building.

- 501_DSP_Compatible/ipcore/clock/clock.v
- 501_DSP_Compatible/ipcore/mul_16X16/rtl/ipm2l_mult_v1_3_mul_16X16.v
- 501_DSP_Compatible/ipcore/mul_16X16/mul_16X16.v
- 501_DSP_Compatible/ipcore/mul_16X16/rtl/ipm2l_apm_data_pipeline/ipm2l_apm_data_pipeline.v
- 501_DSP_Compatible/ipcore/mul_16X16/rtl/ipm2l_apm_data_pipeline/ipm2l_apm_distributed_sdpram.v
- 501_DSP_Compatible/ipcore/mul_16X16/rtl/ipm2l_apm_data_pipeline/ipm2l_apm_distributed_shiftregister.v
- 601_openlooptest/source/clock/clock.v
- 701/source/clock/clock.v
- UnitBoard_DAB_Test_FanBalance/ipcore/clock/clock.v
- UnitBoard_DAB_Test_FanBalance/ipcore/mul_16X16/rtl/ipm2l_mult_v1_3_mul_16X16.v
- UnitBoard_DAB_Test_FanBalance/ipcore/mul_16X16/mul_16X16.v
- UnitBoard_DAB_Test_FanBalance/ipcore/mul_16X16/rtl/ipm2l_apm_data_pipeline/ipm2l_apm_data_pipeline.v
- UnitBoard_DAB_Test_FanBalance/ipcore/mul_16X16/rtl/ipm2l_apm_data_pipeline/ipm2l_apm_distributed_sdpram.v
- UnitBoard_DAB_Test_FanBalance/ipcore/mul_16X16/rtl/ipm2l_apm_data_pipeline/ipm2l_apm_distributed_shiftregister.v
