#include <Spark/device/SpmvDeviceInterface.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void Spmv_dramWrite(
	int64_t param_size_bytes,
	int64_t param_start_bytes,
	const uint8_t *instream_fromcpu) {}

void Spmv_dramRead(
	int64_t param_size_bytes,
	int64_t param_start_bytes,
	uint8_t *outstream_tocpu) {}

void Spmv(
    int64_t param_nIterations,
    int64_t param_nPartitions,
    int64_t param_vectorLoadCycles,
    int64_t param_vectorSize,
    const int64_t *param_colPtrStartAddresses,
    const int32_t *param_colptrSizes,
    const int32_t *param_colptrUnpaddedlengths,
    const int64_t *param_indptrValuesAddresses,
    const int32_t *param_indptrValuesSizes,
    const int32_t *param_indptrValuesUnpaddedLengths,
    const int32_t *param_nrows,
    const int32_t *param_outResultSizes,
    const int64_t *param_outStartAddresses,
    const int32_t *param_paddingCycles,
    const int32_t *param_reductionCycles,
    const int32_t *param_totalCycles,
    const int64_t *param_vStartAddresses) {}

#ifdef __cplusplus
}
#endif /* __cplusplus */
