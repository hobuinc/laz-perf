// Make a LAS file with random data in it.

#include <climits>
#include <string>
#include <vector>
#include <iostream>
#include <random>

#include "io.hpp"
#include "las.hpp"

void outputHelp();
void createFile(const std::string filename, int pdrf, int extra_bytes, double percent);

int main(int argc, char *argv[])
{
    if (argc != 4)
        outputHelp();

    std::string filename = argv[1];
    std::string format_string = argv[2];
    std::string percent_string = argv[3];

    int extra_bytes = 0;
    size_t pos = format_string.find('/');
    if (pos != std::string::npos)
    {
        extra_bytes = std::stoi(format_string.substr(pos + 1));
        format_string = format_string.substr(0, pos);
    }

    size_t cnt;
    int pdrf = std::stoi(format_string, &cnt);
    if (cnt != format_string.size() || pdrf < 0 || pdrf == 4 || pdrf == 5 || pdrf > 8)
    {
        std::cerr << "Invalid LAS format '" << format_string <<
            "'. Must be 0, 1, 2, 3, 6, 7 or 8.\n";
        return -1;
    }

    double percent = std::stod(percent_string, &cnt);
    if (cnt != percent_string.size() || percent < 0 || percent > 100)
    {
        std::cerr << "Invalid random percent '" << percent_string << "'. Must be [0, 100].\n";
        return -1;
    }

    createFile(filename, pdrf, extra_bytes, percent);
}

void outputHelp()
{
    std::cout << "random <filename> <LAS format[/eb count]>  <random percent>\n";
    exit(0);
}

void createFile(const std::string filename, int pdrf, int extra_bytes, double percent)
{
    using namespace lazperf;

    writer::named_file::config c;
    c.minor_version = 4;
    c.pdrf = pdrf;
    c.extra_bytes = extra_bytes;
    c.chunk_size = 0; // Non compressed.

    char buf[100];
    char *pos = buf;
    using GENERATOR = std::mt19937;
    std::random_device rd;
    std::vector<int32_t> seed;
    for (size_t i = 0; i < GENERATOR::state_size; ++i)
        seed.push_back(rd());
    std::seed_seq seedSeq(seed.begin(), seed.end());
    std::mt19937 gen(seedSeq);
    writer::named_file f(filename, c);
    if (pdrf < 6)
    {
        las::point10 *p = reinterpret_cast<las::point10 *>(pos);
        p->x = std::uniform_int_distribution<int32_t>()(gen);
        p->y = std::uniform_int_distribution<int32_t>()(gen);
        p->z = std::uniform_int_distribution<int32_t>()(gen);
        p->intensity = std::uniform_int_distribution<int16_t>()(gen);
        p->return_number = std::uniform_int_distribution<>(0, 7)(gen);
        p->number_of_returns_of_given_pulse = std::uniform_int_distribution<>(0, 7)(gen);
        p->scan_direction_flag = std::uniform_int_distribution<>(0, 1)(gen);
        p->edge_of_flight_line = std::uniform_int_distribution<>(0, 1)(gen);
        p->classification = std::uniform_int_distribution<uint8_t>()(gen);
        p->scan_angle_rank = std::uniform_int_distribution<int8_t>()(gen);
        p->user_data = std::uniform_int_distribution<uint8_t>()(gen);
        p->point_source_ID = std::uniform_int_distribution<uint16_t>()(gen);

        pos += sizeof(las::point10);

        if (pdrf == 1 || pdrf == 3)
        {
            las::gpstime *g = reinterpret_cast<las::gpstime *>(pos);
            g->value = std::uniform_real_distribution<>()(gen);
            pos += sizeof(las::gpstime);
        }
        if (pdrf == 2 || pdrf == 3)
        {
            las::rgb *rgb = reinterpret_cast<las::rgb *>(pos);
            rgb->r = std::uniform_int_distribution<uint16_t>()(gen);
            pos += sizeof(las::rgb);
        }
    }
    else
    {
        las::point14 *p = reinterpret_cast<las::point14 *>(pos);
        p->x_ = std::uniform_int_distribution<int32_t>()(gen);
        p->y_ = std::uniform_int_distribution<int32_t>()(gen);
        p->z_ = std::uniform_int_distribution<int32_t>()(gen);
        p->intensity_ = std::uniform_int_distribution<int16_t>()(gen);
        p->returns_ = std::uniform_int_distribution<uint8_t>()(gen);
        p->flags_ = std::uniform_int_distribution<uint8_t>()(gen);
        p->classification_ = std::uniform_int_distribution<uint8_t>()(gen);
        p->user_data_ = std::uniform_int_distribution<uint8_t>()(gen);
        p->scan_angle_ = std::uniform_int_distribution<int16_t>()(gen);
        p->point_source_ID_ = std::uniform_int_distribution<uint16_t>()(gen);
        p->gpstime_ = std::uniform_real_distribution<>()(gen);
        pos += sizeof(las::point14);

        if (pdrf == 7 || pdrf == 8)
        {
            las::rgb14 *color = reinterpret_cast<las::rgb14 *>(pos);
            color->r = std::uniform_int_distribution<uint16_t>()(gen);
            color->g = std::uniform_int_distribution<uint16_t>()(gen);
            color->b = std::uniform_int_distribution<uint16_t>()(gen);
            pos += sizeof(las::rgb14);

            if (pdrf == 8)
            {
                las::nir14 *nir = reinterpret_cast<las::nir14 *>(pos);
                nir->val = std::uniform_int_distribution<uint16_t>()(gen);
                pos += sizeof(las::nir14);
            }
        }
    }
    for (int i = 0; i < extra_bytes; ++i)
        *pos++ = std::uniform_int_distribution<char>()(gen);

    size_t len = pos - buf;
    size_t bits = len * CHAR_BIT;
    f.writePoint(buf);
    auto dist = std::uniform_int_distribution<>(0, bits - 1);
    for (size_t cnt = 1; cnt < 50000; ++cnt)
    {
        // This won't really flip percent% of bits because we may well choose the
        // same value more than once, but for this use, it seems fine.
        size_t rbits = (percent / 100) * bits;
        while (rbits--)
        {
            int val = dist(gen);
            char& c = *(buf + (val / CHAR_BIT));
            size_t bit = val % CHAR_BIT;
            char mask = (1 << bit);
            if (c & mask)
                c = c & ~mask;
            else
                c = c | mask;
        }
        f.writePoint(buf);
    }

    f.close();
}
