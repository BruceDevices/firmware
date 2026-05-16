// Comment out the next #defines if you are not using an SD Card to store the JPEGs
// Commenting out the line is NOT essential but will save some FLASH space if
// SD Card access is not needed. Note: use of SdFat is currently untested!

#define LOAD_SD_LIBRARY // Default SD Card library
//#define LOAD_SDFAT_LIBRARY // Use SdFat library instead, so SD Card SPI can be bit bashed


// Note for ESP8266 users:
// If the sketch uses SPIFFS and has included FS.h without defining FS_NO_GLOBALS first
// then the JPEGDecoder library will NOT load the SD or SdFat libraries. Use lines thus
// in your sketch (see the examples included in the JPEGDecoder library):
/*
#define FS_NO_GLOBALS
#include <FS.h>

// You will then need to directly reference the SPIFFS File type thus in the sketch, e.g.:

fs::File jpegFile = SPIFFS.open( filename, "r"); // Example

// This will then allow the default method of using the SD library File type to be used
// in the same sketch, e.g.:

File jpegFile = SD.open( filename, FILE_READ);

*/

// This is all to avoid a redefinition of 'class fs::File' error due to a conflict between the
// duplicate definitions in the SD library and the SPIFFS library.
	

#ifdef ARDUINO_ARCH_ESP8266
  // Uncomment out the next #define if you want the bytes swapped in the image blocks
	// returned by read(). 

	// Swapping the bytes is only needed to use the ESP8266 SPI library writePattern()
	// member function and it is better to use readSwappedBytes() instead of read() in
	// the sketch. Images will look psychedelic with wrong colours if the SPI transmit byte
	// order is not right for your sketch!
	
	 #define SWAP_BYTES // Deprecated, only included for backwards compatibility
#endif
