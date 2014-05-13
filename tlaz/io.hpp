// io.hpp
// LAZ file handling
//

#ifndef __io_hpp__
#define __io_hpp__

#include <fstream>
#include <mutex>

#include "formats.hpp"
#include "excepts.hpp"
#include "factory.hpp"
#include "decoder.hpp"

namespace laszip {
	namespace io {
		// LAZ file header
#pragma pack(push, 1)
		struct header {
			char magic[4];
			unsigned short file_source_id;
			unsigned short global_encoding;
			char guid[16];

			struct {
				unsigned char major;
				unsigned char minor;
			} version;

			char system_identifier[32];
			char generating_software[32];

			struct {
				unsigned short day;
				unsigned short year;
			} creation;

			unsigned short header_size;
			unsigned int point_offset;
			unsigned int vlr_count;

			unsigned char point_format_id;
			unsigned short point_record_length;

			unsigned int point_count;
			unsigned int points_by_return[5];

			struct {
				double x, y, z;
			} scale;

			struct {
				double x, y, z;
			} offset;

			struct {
				double x, y, z;
			} min;

			struct {
				double x, y, z;
			} max;
		};

		// A Single LAZ Item representation
		struct laz_item {
			unsigned short type,
						   size,
						   version;
		};

		struct laz_vlr {
			unsigned short compressor;
			unsigned short coder;

			struct {
				unsigned char major;
				unsigned char minor;
				unsigned short revision;
			} version;

			unsigned int options;
			unsigned int chunk_size;

			int64_t num_points,
					num_bytes;

			std::vector<laz_item> items;
		};
#pragma pack(pop)

		// cache line
#define ALIGN 64
#define BUF_SIZE (1 << 20)

		void *aligned_malloc(int size) {
			void *mem = malloc(size+ALIGN+sizeof(void*));
			void **ptr = (void**)((long)(((char*)mem)+ALIGN+sizeof(void*)) & ~(ALIGN-1));
			ptr[-1] = mem;
			return ptr;
		}

		void aligned_free(void *ptr) {
			free(((void**)ptr)[-1]);
		}

		struct __ifstream_wrapper {
			__ifstream_wrapper(std::ifstream& f) : f_(f), offset(0), have(0), 
				buf_((char*)aligned_malloc(BUF_SIZE)) {
			}

			~__ifstream_wrapper() {
				aligned_free(buf_);
			}

			inline void fillit_() {
				if (offset >= have) {
					offset = 0;
					f_.read(buf_, BUF_SIZE);
					have = f_.gcount();
					if (have == 0)
						throw end_of_file(); // this is an exception since we shouldn't be hitting eof
				}
			}

			inline void reset() {
				offset = have = 0; // when a file is seeked, reset this
			}

			inline unsigned char getByte() { 
				fillit_();
				return static_cast<unsigned char>(buf_[offset++]);
			}

			inline void getBytes(unsigned char *buf, size_t len) {
				fillit_();

				std::copy(buf_ + offset, buf_ + offset + len, buf);
				offset += len;
			}

			std::ifstream& f_;
			size_t offset, have;
			char *buf_;
		};

		class file {
			typedef std::function<void (header&)> validator_type;

		public:
			file() : f_(), wrapper_(f_) {
			}
			~file() {
				if (f_.good())
					f_.close();
			}

			explicit file(const std::string& name) : f_(), wrapper_(f_) {
				open(name);
			}

			void open(const std::string& name) {
				f_.open(name, std::ios::binary);

				if (!f_.good())
					throw file_not_found();

				// Make sure our header is correct

				char magic[4];
				f_.read(magic, sizeof(magic));

				if (std::string(magic, magic+4) != "LASF")
					throw invalid_magic();

				// Read the header in
				f_.seekg(0);
				f_.read((char*)&header_, sizeof(header_));

				// The mins and maxes are in a weird order, fix them
				_fixMinMax(header_);

				// make sure everything is valid with the header, note that validators are allowed
				// to manipulate the header, since certain validators depend on a header's orignial state
				// to determine what its final stage is going to be
				for (auto f : _validators())
					f(header_);

				// things look fine, move on with VLR extraction
				_parseLASZIP();

				// parse the chunk table offset
				_parseChunkTable();

				// set the file pointer to the beginning of data to start reading
				f_.clear(); // may have treaded past the EOL, so reset everything before we start reading:w
				f_.seekg(header_.point_offset + sizeof(int64_t));

				wrapper_.reset();
			}

			const header& get_header() const {
				if (!f_.is_open())
					throw invalid_header_request();

				return header_;
			}

			const laz_vlr& get_laz_vlr() const {
				return laz_;
			}

			void readPoint(char *out) {
				// read the next point in
				if (chunk_state_.points_read == laz_.chunk_size ||
					!pdecomperssor_ || !pdecoder_) {
					// Its time to (re)init the decoder
					//
					pdecomperssor_.reset();
					pdecoder_.reset();

					pdecoder_.reset(new decoders::arithmetic<__ifstream_wrapper>(wrapper_));
					pdecomperssor_ = factory::build_decompressor(*pdecoder_, schema_);

					// reset chunk state
					chunk_state_.current++;
					chunk_state_.points_read = 0;
				}

				pdecomperssor_->decompress(out);
				chunk_state_.points_read ++;
			}

		private:
			void _fixMinMax(header& h) {
				double mx, my, mz, nx, ny, nz;

				mx = h.min.x; nx = h.min.y;
				my = h.min.z; ny = h.max.x;
				mz = h.max.y; nz = h.max.z;

				h.min.x = nx; h.max.x = mx;
				h.min.y = ny; h.max.y = my;
				h.min.z = nz; h.max.z = mz;
			}

			void _parseLASZIP() {
				// move the pointer to the begining of the VLR
				f_.seekg(header_.header_size);

#pragma pack(push, 1)
				struct {
					unsigned short reserved;
					char user_id[16];
					unsigned short record_id;
					unsigned short record_length;
					char desc[32];
				} vlr_header;
#pragma pack(pop)

				size_t count = 0;
				bool laszipFound = false;
				while(count < header_.vlr_count && f_.good() && !f_.eof()) {
					f_.read((char*)&vlr_header, sizeof(vlr_header));

					const char *user_id = "laszip encoded";

					if (std::equal(vlr_header.user_id, vlr_header.user_id + 14, user_id) &&
						vlr_header.record_id == 22204) {
						// this is the laszip VLR
						//
						laszipFound = true;

						char *buffer = new char[vlr_header.record_length];

						f_.read(buffer, vlr_header.record_length);
						_parseLASZIPVLR(buffer);

						delete [] buffer;
						break; // no need to keep iterating
					}

					f_.seekg(vlr_header.record_length, std::ios::cur); // jump foward
					count++;
				}

				if (!laszipFound)
					throw no_laszip_vlr();


				// convert the laszip items into record schema to be used by compressor/decompressor
				// builder
				for(auto item : laz_.items) {
					schema_.push(factory::record_item(item.type, item.size, item.version));
				}
			}

			void _parseLASZIPVLR(const char *buf) {
				// read the header information
				//
				std::copy(buf, buf + 32, (char*)&laz_); // don't write to std::vector

				unsigned short num_items = (((unsigned short)buf[33]) << 8) | buf[32];

				// parse and build laz items
				buf += 34;
				for (size_t i = 0 ; i < num_items ; i ++) {
					laz_item item;
					std::copy(buf, buf + 6, (char*)&item);

					laz_.items.push_back(item);

					buf += 6;
				}
			}

			void _parseChunkTable() {
				// Move to the begining of the data
				//
				f_.seekg(header_.point_offset);

				int64_t chunkoffset = 0;
				f_.read((char*)&chunkoffset, sizeof(chunkoffset));
				if (!f_.good())
					throw chunk_table_read_error();

				if (chunkoffset == -1)
					throw not_supported("Chunk table offset == -1 is not supported at this time");

				// Go to the chunk offset and read in the table
				//
				f_.seekg(chunkoffset);
				if (!f_.good())
					throw chunk_table_read_error();

				// Now read in the chunk table
				struct {
					unsigned int version,
								 chunk_count;
				} chunk_table_header;

				f_.read((char *)&chunk_table_header, sizeof(chunk_table_header));
				if (!f_.good())
					throw chunk_table_read_error();

				if (chunk_table_header.version != 0)
					throw unknown_chunk_table_format();

				// start pushing in chunk table offsets
				chunk_table_offsets_.clear();

				if (laz_.chunk_size == std::numeric_limits<unsigned int>::max())
					throw not_supported("chunk_size == uint.max is not supported at this time, call 1-800-DAFUQ for support.");

				// Allocate enough room for our chunk
				chunk_table_offsets_.resize(chunk_table_header.chunk_count);

				// Add The first one
				chunk_table_offsets_[0] = header_.point_offset + sizeof(uint64_t);

				if (chunk_table_header.chunk_count > 1) {
					// decode the index out
					//
					__ifstream_wrapper w(f_);

					decoders::arithmetic<__ifstream_wrapper> decoder(w);
					decompressors::integer decomp(32, 2);

					// start decoder
					decoder.readInitBytes();
					decomp.init();

					int last = 0;
					for (size_t i = 1 ; i <= chunk_table_header.chunk_count ; i ++) {
						last = decomp.decompress(decoder, last, 1);
						chunk_table_offsets_[i] = last + chunk_table_offsets_[i-1];

						//std::cout << "chunk: " << chunk_table_offsets_[i-1] << " --> " << chunk_table_offsets_[i] << std::endl;
					}
				}
			}


			static const std::vector<validator_type>& _validators() {
				static std::vector<validator_type> v; // static collection of validators
				static std::mutex lock;

				// To remain thread safe we need to make sure we have appropriate guards here
				//
				if (v.empty()) {
					lock.lock();
					// Double check here if we're still empty, the first empty just makes sure
					// we have a quick way out where validators are already filled up (for all calls
					// except the first one), for two threads competing to fill out the validators
					// only one of the will get here first, and the second one will bail if the v
					// is not empty, and hence the double check
					//
					if (v.empty()) {
						// TODO: Fill all validators here
						//
						v.push_back(
							// Make sure that the header indicates that file is compressed
							//
							[](header& h) {
								int bit_7 = (h.point_format_id >> 7) & 1,
									bit_6 = (h.point_format_id >> 6) & 1;

								if (bit_7 == 1 && bit_6 == 1)
									throw old_style_compression();

								if ((bit_7 ^ bit_6) == 0)
									throw not_compressed();

								h.point_format_id &= 0x3f;
							}
						);
					}

					lock.unlock();
				}

				return v;
			}

			// The file object is not copyable or copy constructible
			file(const file&) : f_(), wrapper_(f_) {}
			file& operator = (const file&) { return *this;}

			std::ifstream f_;
			__ifstream_wrapper wrapper_;

			header header_;
			laz_vlr laz_;
			std::vector<uint64_t> chunk_table_offsets_;

			factory::record_schema schema_;	// the schema of this file, the LAZ items converted into factory recognizable description,

			// Our decompressor
			std::shared_ptr<decoders::arithmetic<__ifstream_wrapper> > pdecoder_;
			formats::dynamic_decompressor::ptr pdecomperssor_;

			// Establish our current state as we iterate through the file
			struct __chunk_state{
				size_t current;
				size_t points_read;
				size_t current_index;

				__chunk_state() : current(0), points_read(0), current_index(-1) {}
			} chunk_state_;
		};
	}
}

#endif // __io_hpp__
