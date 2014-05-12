// reader.hpp
// Utility reader for LAS/LAZ files
//

#ifndef __reader_hpp__
#define __reader_hpp__

// A simple classes that offsets to the data for us
struct reader {
	reader(const std::string& f) :
		f_(f, std::ios::binary) {
#define good_or_bail(msg) if(!f_.good()) { f_.close(); throw std::runtime_error(f + std::string(" : ") + msg);}

			good_or_bail("Could not open file");

			// read data offset and jump to it
			f_.seekg(32*3); good_or_bail("Could not read data offset");

			unsigned int data = 0;
			f_.read((char*)&data, 4); good_or_bail("Could not read data offset");

			size_ = 0;
			f_.seekg(32*3+8+1); good_or_bail("Could not read record size");
			f_.read((char *)&size_, sizeof(size_)); good_or_bail("Could not read record size");
			f_.read((char *)&count_, sizeof(count_)); good_or_bail("Could not read points count");

			f_.seekg(data); good_or_bail("Could not offset to data offset");
		}

	~reader() {
		f_.close();
	}


	void skip(size_t n) {
		f_.seekg(n, std::ios::cur);
	}

	unsigned char byte() {
		return static_cast<unsigned char>(f_.get());
	}

	void record(char *b) { // make sure you pass enough memory
		f_.read(b, size_);
	}

	std::ifstream f_;
	unsigned short size_;
	unsigned int count_;
};

#endif // __reader_hpp__
