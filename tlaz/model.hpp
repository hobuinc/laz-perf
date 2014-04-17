// model.hpp
//


#ifndef __model_hpp__
#define __model_hpp__

#include "../common/types.hpp"

namespace laszip {
	namespace models {
		struct arithmetic {
			arithmetic(U32 syms, bool com = false) : 
				symbols(syms), compress(com),
				distribution(NULL), symbol_count(NULL), decoder_table(NULL) {
			}

			I32 init(U32* table=0) {
				if (distribution == NULL) {
					if ( (symbols < 2) || (symbols > (1 << 11)) ) {
						return -1; // invalid number of symbols
					}

					last_symbol = symbols - 1;
					if ((!compress) && (symbols > 16)) {
						U32 table_bits = 3;
						while (symbols > (1U << (table_bits + 2))) ++table_bits;
						table_size  = 1 << table_bits;
						table_shift = DM__LengthShift - table_bits;
						distribution = new U32[2*symbols+table_size+2];
						decoder_table = distribution + 2 * symbols;
					}
					else { // small alphabet: no table needed
						decoder_table = 0;
						table_size = table_shift = 0;
						distribution = new U32[2*symbols];
					}

					if (distribution == NULL) {
						return -1; // "cannot allocate model memory");
					}

					symbol_count = distribution + symbols;
				}

				total_count = 0;
				update_cycle = symbols;

				if (table)
					for (U32 k = 0; k < symbols; k++) symbol_count[k] = table[k];
				else
					for (U32 k = 0; k < symbols; k++) symbol_count[k] = 1;

				update();
				symbols_until_update = update_cycle = (symbols + 6) >> 1;

				return 0;
			}

			void update() {
				// halve counts when a threshold is reached
				if ((total_count += update_cycle) > DM__MaxCount) {
					total_count = 0;
					for (U32 n = 0; n < symbols; n++)
					{
						total_count += (symbol_count[n] = (symbol_count[n] + 1) >> 1);
					}
				}

				// compute cumulative distribution, decoder table
				U32 k, sum = 0, s = 0;
				U32 scale = 0x80000000U / total_count;

				if (compress || (table_size == 0)) {
					for (k = 0; k < symbols; k++)
					{
						distribution[k] = (scale * sum) >> (31 - DM__LengthShift);
						sum += symbol_count[k];
					}
				}
				else {
					for (k = 0; k < symbols; k++)
					{
						distribution[k] = (scale * sum) >> (31 - DM__LengthShift);
						sum += symbol_count[k];
						U32 w = distribution[k] >> table_shift;
						while (s < w) decoder_table[++s] = k - 1;
					}
					decoder_table[0] = 0;
					while (s <= table_size) decoder_table[++s] = symbols - 1;
				}

				// set frequency of model updates
				update_cycle = (5 * update_cycle) >> 2;
				U32 max_cycle = (symbols + 6) << 3;

				if (update_cycle > max_cycle) update_cycle = max_cycle;
				symbols_until_update = update_cycle;
			}

			U32 symbols;
			bool compress;

			U32 * distribution, * symbol_count, * decoder_table;

			U32 total_count, update_cycle, symbols_until_update;
			U32 last_symbol, table_size, table_shift;
		};

		struct arithmetic_bit {
			void init() {
				// initialization to equiprobable model
				bit_0_count = 1;
				bit_count   = 2;
				bit_0_prob  = 1U << (BM__LengthShift - 1);
				// start with frequent updates
				update_cycle = bits_until_update = 4;
			}

			void update() {
				// halve counts when a threshold is reached
				if ((bit_count += update_cycle) > BM__MaxCount)
				{
					bit_count = (bit_count + 1) >> 1;
					bit_0_count = (bit_0_count + 1) >> 1;
					if (bit_0_count == bit_count) ++bit_count;
				}

				// compute scaled bit 0 probability
				U32 scale = 0x80000000U / bit_count;
				bit_0_prob = (bit_0_count * scale) >> (31 - BM__LengthShift);

				// set frequency of model updates
				update_cycle = (5 * update_cycle) >> 2;
				if (update_cycle > 64) update_cycle = 64;
				bits_until_update = update_cycle;
			}

			U32 update_cycle, bits_until_update;
			U32 bit_0_prob, bit_0_count, bit_count;
		};
	}
}

#endif // __model_hpp__
