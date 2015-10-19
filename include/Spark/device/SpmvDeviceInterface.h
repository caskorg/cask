#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <cstdint>

#define Spmv_inputWidth (32)
#define Spmv_numPipes (1)
#define Spmv_PCIE_ALIGNMENT (16)
#define Spmv_maxRows (200000)
#define Spmv_cacheSize (2048)


/**
 * \brief Basic static function for the interface 'dramWrite'.
 *
 * \param [in] param_size_bytes Interface Parameter "size_bytes".
 * \param [in] param_start_bytes Interface Parameter "start_bytes".
 * \param [in] instream_fromcpu The stream should be of size param_size_bytes bytes.
 */
void Spmv_dramWrite(
	int64_t param_size_bytes,
	int64_t param_start_bytes,
	const uint8_t *instream_fromcpu);


/**
 * \brief Basic static function for the interface 'dramRead'.
 *
 * \param [in] param_size_bytes Interface Parameter "size_bytes".
 * \param [in] param_start_bytes Interface Parameter "start_bytes".
 * \param [out] outstream_tocpu The stream should be of size param_size_bytes bytes.
 */
void Spmv_dramRead(
	int64_t param_size_bytes,
	int64_t param_start_bytes,
	uint8_t *outstream_tocpu);


/**
 * \brief Basic static function for the interface 'default'.
 *
 * \param [in] param_nIterations Interface Parameter "nIterations".
 * \param [in] param_nPartitions Interface Parameter "nPartitions".
 * \param [in] param_vectorLoadCycles Interface Parameter "vectorLoadCycles".
 * \param [in] param_vectorSize Interface Parameter "vectorSize".
 * \param [in] param_colPtrStartAddresses Interface Parameter array colPtrStartAddresses[] should be of size 1.
 * \param [in] param_colptrSizes Interface Parameter array colptrSizes[] should be of size 1.
 * \param [in] param_colptrUnpaddedlengths Interface Parameter array colptrUnpaddedlengths[] should be of size 1.
 * \param [in] param_indptrValuesAddresses Interface Parameter array indptrValuesAddresses[] should be of size 1.
 * \param [in] param_indptrValuesSizes Interface Parameter array indptrValuesSizes[] should be of size 1.
 * \param [in] param_indptrValuesUnpaddedLengths Interface Parameter array indptrValuesUnpaddedLengths[] should be of size 1.
 * \param [in] param_nrows Interface Parameter array nrows[] should be of size 1.
 * \param [in] param_outResultSizes Interface Parameter array outResultSizes[] should be of size 1.
 * \param [in] param_outStartAddresses Interface Parameter array outStartAddresses[] should be of size 1.
 * \param [in] param_paddingCycles Interface Parameter array paddingCycles[] should be of size 1.
 * \param [in] param_reductionCycles Interface Parameter array reductionCycles[] should be of size 1.
 * \param [in] param_totalCycles Interface Parameter array totalCycles[] should be of size 1.
 * \param [in] param_vStartAddresses Interface Parameter array vStartAddresses[] should be of size 1.
 */
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
	const int64_t *param_vStartAddresses);

#ifdef __cplusplus
}
#endif /* __cplusplus */
