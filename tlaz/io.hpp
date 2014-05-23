/*
===============================================================================

  FILE:  io.hpp
  
  CONTENTS:
    LAZ io

  PROGRAMMERS:

    martin.isenburg@rapidlasso.com  -  http://rapidlasso.com
    uday.karan@gmail.com - Hobu, Inc.
  
  COPYRIGHT:
  
    (c) 2007-2014, martin isenburg, rapidlasso - tools to catch reality
    (c) 2014, Uday Verma, Hobu, Inc.

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
===============================================================================
*/

#ifndef __io_hpp__
#define __io_hpp__

#include <fstream>
#include <string.h>
#include <mutex>

#include "formats.hpp"
#include "excepts.hpp"
#include "factory.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "util.hpp"

namespace laszip {
	// A simple datastructure to get input from the user
	template<
		typename T
	>
	struct vector3 {
		T x, y, z;

		vector3() : x(0), y(0), z(0) {}
		vector3(const T& _x, const T& _y, const T& _z) :
			x(_x), y(_y), z(_z) {
		}

	};

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

			static laz_vlr from_schema(const factory::record_schema& s) {
				laz_vlr r;

				r.compressor = 0;
				r.coder = 0;

				// the version we're compatible with
				r.version.major = 2;
				r.version.minor = 2;
				r.version.revision = 0;

				r.options = 0;
				r.chunk_size = 0;

				r.num_points = -1;
				r.num_bytes = -1;

				for (auto rec : s.records) {
					laz_item i;
					i.type = rec.type;
					i.size = rec.size;
					i.version = rec.version;

					r.items.push_back(i);
				}

				return r;
			}
		};
#pragma pack(pop)

		// cache line
#define BUF_SIZE (1 << 20)

		struct __ifstream_wrapper {
			__ifstream_wrapper(std::ifstream& f) : f_(f), offset(0), have(0), 
				buf_((char*)utils::aligned_malloc(BUF_SIZE)) {
			}

			~__ifstream_wrapper() {
				utils::aligned_free(buf_);
			}

			__ifstream_wrapper(const __ifstream_wrapper&) = delete;
			__ifstream_wrapper& operator = (const __ifstream_wrapper&) = delete;

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
			std::streamsize offset, have;
			char *buf_;
		};

		struct __ofstream_wrapper {
			__ofstream_wrapper(std::ofstream& f) : f_(f) {}

			void putBytes(const unsigned char *b, size_t len) {
				f_.write(reinterpret_cast<const char*>(b), len);
			}

			void putByte(unsigned char b) {
				f_.put((char)b);
			}

			std::ofstream& f_;
		};

		namespace reader {
			class file {
				typedef std::function<void (header&)> validator_type;

				public:
				file() : f_(), wrapper_(f_) {
				}
				~file() {
					close();
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

				void close() {
					if (f_.is_open())
						f_.close();
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
						for (size_t i = 1 ; i <= chunk_table_header.chunk_count - 1 ; i ++) {
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
					int64_t current;
					int64_t points_read;
					int64_t current_index;

					__chunk_state() : current(0u), points_read(0u), current_index(-1) {}
				} chunk_state_;
			};
		}

		namespace writer {
			constexpr unsigned int DefaultChunkSize = 50000;

			// An object to encapsulate what gets passed to 
			struct config {
				vector3<double> scale, offset;
				unsigned int chunk_size;

				explicit config() : scale(1.0, 1.0, 1.0), offset(0.0, 0.0, 0.0), chunk_size(DefaultChunkSize) {}
				explicit config(const vector3<double>& s, const vector3<double>& o, unsigned int cs = DefaultChunkSize) :
					scale(s), offset(o), chunk_size(cs) {}

				header to_header() const {
					header h; memset(&h, 0, sizeof(h)); // clear out header

					h.offset.x = offset.x;
					h.offset.y = offset.y;
					h.offset.z = offset.z;

					h.scale.x = scale.x;
					h.scale.y = scale.y;
					h.scale.z = scale.z;

					return h;
				}
			};

			class file {
			public:
				file() :
					wrapper_(f_) {}

				file(const std::string& filename,
						const factory::record_schema& s,
						const config& config) :
					wrapper_(f_),
					schema_(s),
					header_(config.to_header()),
					chunk_size_(config.chunk_size) {
						open(filename, s, config);
				}

				void open(const std::string& filename, const factory::record_schema& s, const config& c) {
					// open the file and move to offset of data, we'll write
					// headers and all other things on file close
					f_.open(filename, std::ios::binary);
					if (!f_.good())
						throw write_open_failed();

					schema_ = s;
					header_ = c.to_header();
					chunk_size_ = c.chunk_size;

					// write junk to our prelude, we'll overwrite this with
					// awesome data later
					//
					size_t preludeSize = 
						sizeof(header) +	// the LAS header
						(34 + s.records.size() * 6) + // the LAZ vlr size 
						sizeof(int64_t); // chunk table offset

					char *junk = new char[preludeSize];
					std::fill(junk, junk + preludeSize, 0);
					f_.write(junk, preludeSize);
					delete [] junk;
				}

				void writePoint(const char *p) {
					if (chunk_state_.points_in_chunk == chunk_size_ ||
							!pcompressor_ || !pencoder_) {
						// Time to (re)init the encoder
						//
						pcompressor_.reset();
						if (pencoder_) { 
							pencoder_->done(); // make sure we flush it out
							pencoder_.reset();
						}

						// take note of the current offset
						chunk_offsets_.push_back(f_.tellp());

						// reinit stuff
						pencoder_.reset(new encoders::arithmetic<__ofstream_wrapper>(wrapper_));
						pcompressor_ = factory::build_compressor(*pencoder_, schema_);

						// reset chunk state
						//
						chunk_state_.current_chunk_index ++;
						chunk_state_.points_in_chunk = 0;
					}

					// now write the point
					pcompressor_->compress(p);
					chunk_state_.total_written ++;
					chunk_state_.points_in_chunk ++;


					_update_min_max(*(reinterpret_cast<const formats::las::point10*>(p)));
				}

				void close() {
					_flush();

					if (f_.is_open())
						f_.close();
				}

			private:
				void _update_min_max(const formats::las::point10& p) {
					double x = p.x * header_.scale.x + header_.offset.x,
						   y = p.y * header_.scale.y + header_.offset.y,
						   z = p.z * header_.scale.z + header_.offset.z;

					header_.min.x = std::min(x, header_.min.x);
					header_.min.y = std::min(y, header_.min.y);
					header_.min.z = std::min(z, header_.min.z);

					header_.max.x = std::max(x, header_.max.x);
					header_.max.y = std::max(y, header_.max.y);
					header_.max.z = std::max(z, header_.max.z);
				}

				void _flush() {
					// flush out the encoder
					pencoder_->done();

					// note down the end of file marker, this is where we'll be writing our
					// chunk table
					std::streamsize chunk_table_offset = f_.tellp();

					// Time to write our header
					// Fill up things not filled up by our header
					//
					header_.magic[0] = 'L'; header_.magic[1] = 'A';
					header_.magic[2] = 'S'; header_.magic[3] = 'F'; 

					header_.version.major = 1;
					header_.version.minor = 2;

					header_.header_size = sizeof(header_);
					header_.point_offset = sizeof(header) + (34 + schema_.records.size() * 6);
					header_.vlr_count = 1;

					header_.point_format_id = factory::schema_to_point_format(schema_);
					header_.point_format_id |= (1 << 7);
					header_.point_record_length = schema_.size_in_bytes();
					header_.point_count = chunk_state_.total_written;

					// make sure we re-arrange mins and maxs for writing
					//
					double mx, my, mz, nx, ny, nz;
					nx = header_.min.x; mx = header_.max.x;
					ny = header_.min.y; my = header_.max.y;
					nz = header_.min.z; mz = header_.max.z;

					header_.min.x = mx; header_.min.y = nx;
					header_.min.z = my; header_.max.x = ny;
					header_.max.y = mz; header_.max.z = nz;

					f_.seekp(0);
					f_.write(reinterpret_cast<char*>(&header_), sizeof(header_));

					// before we can write the VLR, we need to write the LAS VLR definition
					// for it
					//
#pragma pack(push, 1)
					struct {
						unsigned short reserved;
						char user_id[16];
						unsigned short record_id;
						unsigned short record_length_after_header;
						char description[32];
					} las_vlr_header;
#pragma pack(pop)

					las_vlr_header.reserved = 0;
					las_vlr_header.record_id = 22204;
					las_vlr_header.record_length_after_header = 34 + (schema_.records.size() * 6);

					strcpy(las_vlr_header.user_id, "laszip encoded");
					strcpy(las_vlr_header.description, "tlaz variant");

					// write the las vlr header
					f_.write(reinterpret_cast<char*>(&las_vlr_header), sizeof(las_vlr_header));


					// prep our VLR so we can write it
					//
					laz_vlr vlr = laz_vlr::from_schema(schema_);

					// write the base header
					f_.write(reinterpret_cast<char*>(&vlr), 32); // don't write the std::vector at the end of the class

					// write the count of items
					unsigned short length = static_cast<unsigned short>(vlr.items.size());
					f_.write(reinterpret_cast<char*>(&length), sizeof(unsigned short));

					// write items
					for (auto i : vlr.items) {
						f_.write(reinterpret_cast<char*>(&i), sizeof(laz_item));
					}

					// TODO: Write chunk table
					//
					_writeChunks();
				}

				void _writeChunks() {
					// move to the end of the file to start emitting our compresed table
					f_.seekp(0, std::ios::end);
					
					// take note of where we're writing the chunk table, we need this later
					int64_t chunk_table_offset = static_cast<int64_t>(f_.tellp());

					// write out the chunk table header (version and total chunks)
#pragma pack(push, 1)
					struct {
						unsigned int version,
									 chunks_count;
					} chunk_table_header = { 0, static_cast<unsigned int>(chunk_offsets_.size()) };
#pragma pack(pop)

					f_.write(reinterpret_cast<char*>(&chunk_table_header),
							 sizeof(chunk_table_header));


					// Now compress and write the chunk table
					//
					__ofstream_wrapper w(f_);

					encoders::arithmetic<__ofstream_wrapper> encoder(w);
					compressors::integer comp(32, 2);

					comp.init();

					for (size_t i = 0 ; i < chunk_offsets_.size() ; i ++) {
						comp.compress(encoder,
								i ? chunk_offsets_[i-1] : 0,
								chunk_offsets_[i], 1);
					}

					encoder.done();

					// go back to where we're supposed to write chunk table offset
					f_.seekp(header_.point_offset);
					f_.write(reinterpret_cast<char*>(&chunk_table_offset), sizeof(chunk_table_offset));
				}

				std::ofstream f_;
				__ofstream_wrapper wrapper_;

				formats::dynamic_compressor::ptr pcompressor_;
				std::shared_ptr<encoders::arithmetic<__ofstream_wrapper> > pencoder_;

				factory::record_schema schema_;

				header header_;
				unsigned int chunk_size_;

				struct __chunk_state {
					int64_t total_written; // total points written
					int64_t current_chunk_index; //  the current chunk index we're compressing
					unsigned int points_in_chunk;
					__chunk_state() : total_written(0), current_chunk_index(-1), points_in_chunk(0) {}
				} chunk_state_;

				std::vector<int64_t> chunk_offsets_; // all the places where chunks begin
			};
		}
	}
}

#endif // __io_hpp__
