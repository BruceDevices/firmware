#include "FEC.h"
#include <string.h>

RadioLibBCH::RadioLibBCH() {
  
}

RadioLibBCH::~RadioLibBCH() {
  #if !RADIOLIB_STATIC_ONLY
    delete[] this->alphaTo;
    delete[] this->indexOf;
    delete[] this->generator;
  #endif
}

/*
  BCH Encoder based on https://www.codeproject.com/articles/13189/pocsag-encoder

  Significantly cleaned up and slightly fixed.
*/
void RadioLibBCH::begin(uint8_t n, uint8_t k, uint32_t poly) {
  this->n = n;
  this->k = k;
  this->poly = poly;
  #if !RADIOLIB_STATIC_ONLY
  this->alphaTo = new int32_t[n + 1];
  this->indexOf = new int32_t[n + 1];
  this->generator = new int32_t[n - k + 1];
  #endif

  // find the maximum power of the polynomial
  for(this->m = 0; this->m < 31; this->m++) {
    if((poly >> this->m) == 1) {
      break;
    }
  }

  /*
  * generate GF(2**m) from the irreducible polynomial p(X) in p[0]..p[m]
  * lookup tables:  index->polynomial form   this->alphaTo[] contains j=alpha**i;
  * polynomial form -> index form  this->indexOf[j=alpha**i] = i alpha=2 is the
  * primitive element of GF(2**m)
  */

	int32_t mask = 1;
	this->alphaTo[this->m] = 0;

	for(uint8_t i = 0; i < this->m; i++) {
		this->alphaTo[i] = mask;

		this->indexOf[this->alphaTo[i]] = i;

    if(this->poly & ((uint32_t)0x01 << i)) {
      this->alphaTo[this->m] ^= mask;
    }

		mask <<= 1;
	}

	this->indexOf[this->alphaTo[this->m]] = this->m;
	mask >>= 1;

	for(uint8_t i = this->m + 1; i < this->n; i++) {
		if(this->alphaTo[i - 1] >= mask) {
      this->alphaTo[i] = this->alphaTo[this->m] ^ ((this->alphaTo[i - 1] ^ mask) << 1);
    } else {
      this->alphaTo[i] = this->alphaTo[i - 1] << 1;
    }

		this->indexOf[this->alphaTo[i]] = i;
	}

	this->indexOf[0] = -1;

  /*
	* Compute generator polynomial of BCH code of length = 31, redundancy = 10
	* (OK, this is not very efficient, but we only do it once, right? :)
	*/

	int32_t ii = 0;
  int32_t jj = 1;
  int32_t ll = 0;
  int32_t kaux = 0;
  bool test = false;
	int32_t aux = 0;
	int32_t cycle[15][6] = { { 0 } };
  int32_t size[15] = { 0 };

	// Generate cycle sets modulo 31
	cycle[0][0] = 0; size[0] = 1;
	cycle[1][0] = 1; size[1] = 1;

	do {
		// Generate the jj-th cycle set
		ii = 0;
		do {
			ii++;
			cycle[jj][ii] = (cycle[jj][ii - 1] * 2) % this->n;
			size[jj]++;
			aux = (cycle[jj][ii] * 2) % this->n;
		} while(aux != cycle[jj][0]);

		// Next cycle set representative
		ll = 0;
		do {
			ll++;
			test = false;
			for(ii = 1; ((ii <= jj) && !test); ii++) {
        // Examine previous cycle sets
			  for(kaux = 0; ((kaux < size[ii]) && !test); kaux++) {
          test = (ll == cycle[ii][kaux]);
        }
      }
		} while(test && (ll < (this->n - 1)));

		if(!test) {
			jj++;	// next cycle set index
			cycle[jj][0] = ll;
			size[jj] = 1;
		}

	} while(ll < (this->n - 1));

	// Search for roots 1, 2, ..., m-1 in cycle sets
	int32_t rdncy = 0;
  #if RADIOLIB_STATIC_ONLY
    int32_t min[RADIOLIB_BCH_MAX_N - RADIOLIB_BCH_MAX_K + 1] = { 0 };
  #else
    int32_t* min = new int32_t[this->n - this->k + 1];
  #endif
	kaux = 0;

  // ensure the first element is always initializer
  min[0] = 0;

	for(ii = 1; ii <= jj; ii++) {
		min[kaux] = 0;
		for(jj = 0; jj < size[ii]; jj++) {
      for(uint8_t root = 1; root < this->m; root++) {
        if(root == cycle[ii][jj]) {
          min[kaux] = ii;
        }
      }
    }

		if(min[kaux]) {
			rdncy += size[min[kaux]];
			kaux++;
		}
	}

	int32_t noterms = kaux;
  #if RADIOLIB_STATIC_ONLY
    int32_t zeros[RADIOLIB_BCH_MAX_N - RADIOLIB_BCH_MAX_K + 1] = { 0 };
  #else
    int32_t* zeros = new int32_t[this->n - this->k + 1];
  #endif
	kaux = 1;

  // ensure the first element is always initializer
  zeros[1] = 0;

	for(ii = 0; ii < noterms; ii++) {
    for(jj = 0; jj < size[min[ii]]; jj++) {
			zeros[kaux] = cycle[min[ii]][jj];
			kaux++;
		}
  }

  #if !RADIOLIB_STATIC_ONLY
  delete[] min;
  #endif

	// Compute generator polynomial
	this->generator[0] = this->alphaTo[zeros[1]];
	this->generator[1] = 1;		// g(x) = (X + zeros[1]) initially

	for(ii = 2; ii <= rdncy; ii++) {
	  this->generator[ii] = 1;
	  for(jj = ii - 1; jj > 0; jj--) {
      if(this->generator[jj] != 0) {
        this->generator[jj] = this->generator[jj - 1] ^ this->alphaTo[(this->indexOf[this->generator[jj]] + zeros[ii]) % this->n];
      } else {
        this->generator[jj] = this->generator[jj - 1];
      }
    }
		this->generator[0] = this->alphaTo[(this->indexOf[this->generator[0]] + zeros[ii]) % this->n];
	}

  #if !RADIOLIB_STATIC_ONLY
  delete[] zeros;
  #endif
}

/*
  BCH Encoder based on https://www.codeproject.com/articles/13189/pocsag-encoder

  Significantly cleaned up and slightly fixed.
*/
uint32_t RadioLibBCH::encode(uint32_t dataword) {
  // we only use the "k" most significant bits
  #if RADIOLIB_STATIC_ONLY
    int32_t data[RADIOLIB_BCH_MAX_K] = { 0 };
  #else
    int32_t* data = new int32_t[this->k];
    memset(data, 0, this->k*sizeof(int32_t));
  #endif
	int32_t j1 = 0;
	for(int32_t i = this->n; i > (this->n - this->k); i--) {
		if(dataword & ((uint32_t)1<<i)) {
      data[j1++]=1;
    } else {
      data[j1++]=0;
    }
	}

  // reset the M(x)+r array elements
  #if RADIOLIB_STATIC_ONLY
    int32_t Mr[RADIOLIB_BCH_MAX_N] = { 0 };
  #else
    int32_t* Mr = new int32_t[this->n];
    memset(Mr, 0x00, this->n*sizeof(int32_t));
  #endif

  // copy the contents of data into Mr and add the zeros
  memcpy(Mr, data, this->k*sizeof(int32_t));

  int32_t j = 0;
  int32_t start = 0;
  int32_t end = this->n - this->k;
  while(end < this->n) {
    for(int32_t i = end; i > start-2; --i) {
      if(Mr[start]) {
        Mr[i] ^= this->generator[j];
        ++j;
      } else {
        ++start;
        j = 0;
        end = start + this->n - this->k;
        break;
      }
    }
  }

  #if RADIOLIB_STATIC_ONLY
    int32_t bb[RADIOLIB_BCH_MAX_N - RADIOLIB_BCH_MAX_K + 1] = { 0 };
  #else
    int32_t* bb = new int32_t[this->n - this->k + 1];
    memset(bb, 0, (this->n - this->k + 1)*sizeof(int32_t));
  #endif
  j = 0;
  for(int32_t i = start; i < end; ++i) {
    bb[j] = Mr[i];
    ++j;
  }

  #if !RADIOLIB_STATIC_ONLY
  delete[] Mr;
  #endif

	int32_t iEvenParity = 0;
  #if RADIOLIB_STATIC_ONLY
    int32_t recd[RADIOLIB_BCH_MAX_N + 1];
  #else
    int32_t* recd = new int32_t[this->n + 1];
  #endif
	for(uint8_t i = 0; i < this->k; i++) {
		recd[this->n - i] = data[i];
		if(data[i] == 1) {
      iEvenParity++;
    }
	}
  
  #if !RADIOLIB_STATIC_ONLY
  delete[] data;
  #endif

	for(uint8_t i = 0; i < this->n - this->k + 1; i++) {
		recd[this->n - this->k - i] = bb[i];
		if(bb[i] == 1) {
      iEvenParity++;
    }
	}
  
  #if !RADIOLIB_STATIC_ONLY
  delete[] bb;
  #endif

	if((iEvenParity % 2) == 0) {
    recd[0] = 0;
  } else {
    recd[0] = 1;
  }

  int32_t res = 0;
	for(int32_t i = 0; i < this->n + 1; i++) {
		if(recd[i]) {
      res |= ((uint32_t)1<<i);
    }
	}

  #if !RADIOLIB_STATIC_ONLY
  delete[] recd;
  #endif

	return(res);
}

RadioLibBCH RadioLibBCHInstance;

RadioLibConvCode::RadioLibConvCode() {

}

void RadioLibConvCode::begin(uint8_t rt) {
  this->enc_state = 0;
  this->rate = rt;
}

int16_t RadioLibConvCode::encode(const uint8_t* in, size_t in_bits, uint8_t* out, size_t* out_bits) {
  if(!in || !out) {
    return(RADIOLIB_ERR_UNKNOWN);
  }

  size_t ind_bit;
  uint16_t data_out_bitcount = 0;
  uint32_t bin_out_word = 0;

  // iterate over the provided bits
  for(ind_bit = 0; ind_bit < in_bits; ind_bit++) {
    uint8_t cur_bit = GET_BIT_IN_ARRAY_LSB(in, ind_bit);
    const uint32_t* lut_ptr = (this->rate == 2) ? ConvCodeTable1_2 : ConvCodeTable1_3;
    uint8_t word_pos = this->enc_state / 4;
    uint8_t byte_pos = (3 - (this->enc_state % 4)) * 8;
    uint8_t nibble_pos = (1 - cur_bit) * 4;
    uint8_t g1g0 = (lut_ptr[word_pos] >> (byte_pos + nibble_pos)) & 0x0F;

    uint8_t mod = this->rate == 2 ? 16 : 64;
    this->enc_state = (this->enc_state * 2 + cur_bit) % mod;
    bin_out_word |= (g1g0 << ((7 - (ind_bit % 8)) * this->rate));
    if(ind_bit % 8 == 7) {
      if(this->rate == 3) {
        *out++ = (uint8_t)(bin_out_word >> 16);
      }
      *out++ = (uint8_t)(bin_out_word >> 8);
      *out++ = (uint8_t)bin_out_word;
      bin_out_word  = 0;
    }
    data_out_bitcount += this->rate;
  }

  if(ind_bit % 8) {
    if(this->rate == 3) {
      *out++ = (uint8_t)(bin_out_word >> 16);
    }
    *out++ = (uint8_t)(bin_out_word >> 8);
    *out++ = (uint8_t)bin_out_word;
  }

  if(out_bits) { *out_bits = data_out_bitcount; }

  return(RADIOLIB_ERR_NONE);
}

RadioLibConvCode RadioLibConvCodeInstance;
