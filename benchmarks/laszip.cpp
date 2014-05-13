// laszip.cpp
// Comparison with laszip
//


#include <iostream>
#include <stdint.h>
#include <string>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <vector>


#include <laszip/laszip.hpp>    
#include <laszip/lasunzipper.hpp>
#include <laszip/laszipper.hpp>

#include "../common/common.hpp"

#include "io.hpp"

// Each library will decode autzen.las without doing any IO to write out points
// The timing will not include time required to initialize encoding etc.
float bench_laszip();
float bench_tlaz();

int main() {
	std::vector<float> laztimes, tlaztimes;
	// run one kind of test at a time, to give them maximum locality of reference benefit

	std::cout << "Benchmarking laszip: " << std::endl;
	for (int i = 0 ; i < 5 ; i ++) {
		std::cout << (i+1) << ": ";

		if (i >= 2) {
			std::cout << "    Capturing..." << std::endl;
			laztimes.push_back(bench_laszip());
		}
		else {
			std::cout << "    Warming up..." << std::endl;
			bench_laszip();
		}
	}

	std::cout << "Benchmarking tlaz: " << std::endl;
	for (int i = 0 ; i < 5 ; i ++) {
		std::cout << (i+1) << ": ";

		if (i >= 2) {
			std::cout << "    Capturing..." << std::endl;
			tlaztimes.push_back(bench_tlaz());
		}
		else {
			std::cout << "    Warming up..." << std::endl;
			bench_tlaz();
		}
	}


	printf("\n\n");
	printf("%10s%10s%10s\n", "Index", "LASzip", "tlaz");
	for (size_t i = 0 ; i < 3 ; i ++) {
		printf("%10zu%10.6f%10.6f\n", i, laztimes[i], tlaztimes[i]);
	}

	return 0;
}

template<class T>
static inline void read_array_field(uint8_t*& src, T* dest, std::size_t count) {
	memcpy((uint8_t*)dest, (uint8_t*)(T*)src, sizeof(T)*count);
	src += sizeof(T) * count;
	return;
}

template <typename T>
static inline size_t read_n(T& dest, FILE* src, size_t const& num) {
	return  fread(dest, 1, num, src);
}

template<class T>
static inline T read_field(uint8_t*& src) {
	T tmp = *(T*)(void*)src;
	src += sizeof(T);
	return tmp;
}


class VLR {
	public:
		uint16_t reserved;
		std::string userId;
		uint16_t recordId;
		uint16_t size;
		std::string description;
		uint8_t* data;
		enum
		{
			eHeaderSize = 54,
			eUserIdSize = 16,
			eDescriptionSize = 32
		};


		VLR()
			: reserved(0)
			  , userId("")
			  , recordId(0)
			  , size(0)
			  , description("")
			  , data(0) {}
		~VLR()
		{
			delete[] data;
		}
		inline void read(FILE* fp) {
			// assumes the stream is already positioned to the beginning

			{
				uint8_t* buf1 = new uint8_t[eHeaderSize];
				size_t numRead = read_n(buf1, fp, eHeaderSize);
				if (numRead != eHeaderSize)
				{
					std::ostringstream oss;
					oss << " read size was invalid, not " << eHeaderSize << std::endl;
				}
				uint8_t* p1 = buf1;

				reserved = read_field<uint16_t>(p1);

				uint8_t userId_data[eUserIdSize];
				read_array_field(p1, userId_data, eUserIdSize);
				userId = std::string((const char*)&userId_data);

				recordId = read_field<uint16_t>(p1);
				size = read_field<uint16_t>(p1);

				uint8_t description_data[eDescriptionSize];
				read_array_field(p1, description_data, eDescriptionSize);
				description = std::string(  (const char*)&description_data);

				delete[] buf1;
			}

			data = new uint8_t[size];
			{
				read_n(data, fp, size);
			}

			return;
		}    
};

// LASzip stuff
std::vector<VLR*> readVLRs(FILE* fp, int count)
{
    std::vector<VLR*> output;
    
    for (int i = 0; i < count; ++i)
    {
        // std::cout << "Reading vlr #:" << i << std::endl;
        VLR* vlr = new VLR;
        vlr->read(fp);
        output.push_back(vlr);
    }
    return output;
}

VLR* getLASzipVLR(std::vector<VLR*> const& vlrs)
{
    std::string userId("laszip encoded");
    uint16_t recordId(22204);
    
    for(size_t i = 0; i < vlrs.size(); ++i)
    {
        VLR* vlr = vlrs[i];
        std::string const& uid = vlr->userId;
        uint16_t rid = vlr->recordId;
        // 
        // std::cout << "VLR recordId: " << rid << std::endl;
        // std::cout << "VLR userid: '" << uid <<"'"<< std::endl;
        // std::cout << "uid size" << uid.size() << std::endl;
        // 
        // std::cout << "uid equal: " << boost::iequals(uid, userId) << std::endl;
        // std::cout << "rid equal: " << (rid == recordId) << std::endl;
        
        if (uid == userId && rid == recordId)
            return vlr;
    }
    return 0;
}

class LASHeader
{
public:
    uint32_t point_count;
    uint32_t vlr_count;
    uint16_t header_size;
    uint32_t data_offset;
    uint8_t point_format_id;
    uint16_t point_record_length;
    double scale[3];
    double offset[3];
    double maxs[3];
    double mins[3];

    LASHeader()
        : point_count (0)
        , vlr_count (0)
        , header_size(0)
        , data_offset(0)
        , point_format_id(0)
        , point_record_length(0)
    {
        for (int i = 0; i < 3; ++i)
        {
            scale[i] = 0.0;
            offset[i] = 0.0;
            mins[i] = 0.0;
            maxs[i] = 0.0;
        }
    }
};

/// The Instance class.  One of these exists for each instance of your NaCl
/// module on the web page.  The browser will ask the Module object to create
/// a new Instance for each occurrence of the <embed> tag that has these
/// attributes:
///     src="hello_tutorial.nmf"
///     type="application/x-pnacl"
/// To communicate with the browser, you must override HandleMessage() to
/// receive messages from the browser, and use PostMessage() to send messages
/// back to the browser.  Note that this interface is asynchronous.
class LASzipInstance {
 public:
  /// The constructor creates the plugin-side instance.
  /// @param[in] instance the handle to the browser-side plugin instance.
  explicit LASzipInstance() :
        fp_(0)
      , bDidReadHeader_(false)
      , pointIndex_(0)
      , point_(0)
      , bytes_(0)
  {
  }

  virtual ~LASzipInstance() 
  {
  }

    bool open(const std::string& filename)
    {
        fp_ = fopen(filename.c_str(), "r");
        if (!fp_)
        {
            return false;
        }
        
        char magic[4] = {'\0'};
        int numRead = fread(&magic, 1, 4, fp_);
        if (numRead != 4)
        {
            return false;
        }
        if (magic[0] != 'L' &&
            magic[1] != 'A' &&
            magic[2] != 'S' &&
            magic[3] != 'F'
            )
        {
            return false;
        }
        
        fseek(fp_, 0, SEEK_SET);

		readHeader(header_);

        return true;
    }
    
    bool readHeader(LASHeader& header)
    {
        fseek(fp_, 32*3 + 11, SEEK_SET);
        size_t result = fread(&header.point_count, 4, 1, fp_);
        if (result != 1)
        {
            return false;
        }
    
        fseek(fp_, 32*3, SEEK_SET);
        result = fread(&header.data_offset, 4, 1, fp_);
        if (result != 1)
        {
            return false;
        }

        fseek(fp_, 32*3+4, SEEK_SET);
        result = fread(&header.vlr_count, 4, 1, fp_);
        if (result != 1)
        {
            return false;
        }
    

        fseek(fp_, 32*3-2, SEEK_SET);
        result = fread(&header.header_size, 2, 1, fp_);
        if (result != 1)
        {
            return false;
        }

        fseek(fp_, 32*3 + 8, SEEK_SET);
        result = fread(&header.point_format_id, 1, 1, fp_);
        if (result != 1)
        {
            return false;
        }

        uint8_t compression_bit_7 = (header.point_format_id & 0x80) >> 7;
        uint8_t compression_bit_6 = (header.point_format_id & 0x40) >> 6;
        
        if (!compression_bit_7 && !compression_bit_6 )
        {
            return false;
        }
        if (compression_bit_7 && compression_bit_6)
        {
            return false;            
        }

        header.point_format_id = header.point_format_id & 0x3f;

        fseek(fp_, 32*3 + 8+1, SEEK_SET);
        result = fread(&header.point_record_length, 2, 1, fp_);
        if (result != 1)
        {
            return false;
        }        


        size_t start = 32*3 + 35;
        fseek(fp_, start, SEEK_SET);

        result = fread(&header.scale, 8, 3, fp_ );
        if (result != 3)
        {       
            return false;
        }
        

        result = fread(&header.offset, 8, 3, fp_ );
        if (result != 3)
        {      
            return false;
        }
        
        result = fread(&header.maxs[0], 8, 1, fp_ );
        if (result != 1)
        {      
            return false;
        }

        result = fread(&header.mins[0], 8, 1, fp_ );
        if (result != 1)
        {      
            return false;
        }
        result = fread(&header.maxs[1], 8, 1, fp_ );
        if (result != 1)
        {      
            return false;
        }
        result = fread(&header.mins[1], 8, 1, fp_ );
        if (result != 1)
        {      
            return false;
        }

        result = fread(&header.maxs[2], 8, 1, fp_ );
        if (result != 1)
        {      
            return false;
        }
        result = fread(&header.mins[2], 8, 1, fp_ );
        if (result != 1)
        {      
            return false;
        }


        fseek(fp_, header_.header_size, SEEK_SET);
        std::vector<VLR*> vlrs = readVLRs(fp_, header_.vlr_count);
        VLR* zvlr = getLASzipVLR(vlrs);
    
        if (!zvlr)
        {
            return false;
        }
        bool stat = zip_.unpack(&(zvlr->data[0]), zvlr->size);
        if (!stat)
        {
            return false;
        }
    
        fseek(fp_, header_.data_offset, SEEK_SET);
        stat = unzipper_.open(fp_, &zip_);
        if (!stat)
        {       
            return false;
        }
        
        for (size_t i = 0; i < vlrs.size(); ++i)
        {
            delete vlrs[i];
        }
        unsigned int point_offset(0); 
        point_ = new unsigned char*[zip_.num_items];
        uint32_t point_size(0);
        for (unsigned i = 0; i < zip_.num_items; i++)
        {
            point_size += zip_.items[i].size;
        }

        bytes_ = new uint8_t[ point_size ];

        for (unsigned i = 0; i < zip_.num_items; i++)
        {
            point_[i] = &(bytes_[point_offset]);
            point_offset += zip_.items[i].size;
        }
                
        return true;
    }

	void close() {
		int res = fclose(fp_);
		header_ = LASHeader();

		zip_ = LASzip();
		unzipper_ = LASunzipper();
		fp_ = NULL;
		bDidReadHeader_ = 0;
		pointIndex_ = 0;


		delete[] point_; point_ = 0;

		delete[] bytes_; bytes_ = 0;
	}


  void readPoint() 
  {
	  bool ok = unzipper_.read(point_);
	  pointIndex_++;
  }

  pthread_t message_thread_;
  bool bCreatedFS_;
  LASzip zip_;
  LASunzipper unzipper_;
  FILE* fp_;
  LASHeader header_;
  bool bDidReadHeader_;
  uint32_t pointIndex_;
  unsigned char** point_;
  unsigned char* bytes_;
};


float bench_laszip() {
	float t;

	LASzipInstance l;
	l.open("test/raw-sets/autzen.laz");

	unsigned int limit = l.header_.point_count;

	auto start = common::tick();
	while(l.pointIndex_ != limit)
		l.readPoint();

	t = common::since(start);
	l.close();

	return t;
}


float bench_tlaz() {
	laszip::io::file f("test/raw-sets/autzen.laz");
	float t;

	size_t limit = f.get_header().point_count;
	char buf[256]; // a buffer large enough to hold our point


	auto start = common::tick();
	for (size_t i = 0 ; i < limit ; i ++) 
		f.readPoint(buf);

	t = common::since(start);
	return t;
}
