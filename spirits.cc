//
// spirits-vna-fun
// https://github.com/mistine/spirits-vna-fun
//

//
// To build this you'll need nlohmann/json and yhirose/cpp-httplib included.
// Too lazy to add submodules.
//

#include <vector>                  // std::vector
#include <iostream>                // std::ifstream, std::ofstream
#include <fstream>
#include <string>                  // std::string
#include <iomanip>                 // std::quoted
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#endif
#include <httplib.h>
#include <json.hpp>
#include <thread>
using nlohmann::json;

static std::string base64_encode(const std::string data) 
{
	static constexpr char sEncodingTable[] = {
	  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	  'w', 'x', 'y', 'z', '0', '1', '2', '3',
	  '4', '5', '6', '7', '8', '9', '+', '/'
	};

	size_t in_len = data.size();
	size_t out_len = 4 * ((in_len + 2) / 3);
	std::string ret(out_len, '\0');
	size_t i;
	char* p = const_cast<char*>(ret.c_str());

	for (i = 0; i < in_len - 2; i += 3) {
		*p++ = sEncodingTable[(data[i] >> 2) & 0x3F];
		*p++ = sEncodingTable[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
		*p++ = sEncodingTable[((data[i + 1] & 0xF) << 2) | ((int)(data[i + 2] & 0xC0) >> 6)];
		*p++ = sEncodingTable[data[i + 2] & 0x3F];
	}
	if (i < in_len) {
		*p++ = sEncodingTable[(data[i] >> 2) & 0x3F];
		if (i == (in_len - 1)) {
			*p++ = sEncodingTable[((data[i] & 0x3) << 4)];
			*p++ = '=';
		}
		else {
			*p++ = sEncodingTable[((data[i] & 0x3) << 4) | ((int)(data[i + 1] & 0xF0) >> 4)];
			*p++ = sEncodingTable[((data[i + 1] & 0xF) << 2)];
		}
		*p++ = '=';
	}

	return ret;
}

float random_float(float min, float max)
{
	std::random_device device;
	std::mt19937 engine(device());
	std::uniform_real_distribution<> distribution(min, max);
	return static_cast<float>(distribution(engine));
}

int random_int(int min, int max)
{
	std::random_device device;
	std::mt19937 engine(device());
	std::uniform_int_distribution<int> distribution(min, max);
	return static_cast<int>(distribution(engine));
}

std::string random_string(size_t length)
{
	auto randchar = []() -> char {
		std::string charset = "0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz";
		return charset.at(random_int(0, charset.length()));
	};
	std::string str(length, 0);
	std::generate_n(str.begin(), length, randchar);
	return str;
}

static int total_views = 0, total_likes = 0;
int main(int argc, char** argv)
{
	const std::string domain{ "spirit.vietnamairlines.com" };

	// Parses config (the lazy way)
	std::ifstream cfg_file("config.json"); json config; cfg_file >> config;

	std::string
		url           = config["url"],
		absolute_path = url.substr(url.find(domain) + domain.length()),
		user_agent    = config["user_agent"];
	int
		like_luckness = config["like_luckness_in_percentage"],
		min_sleep     = config["minimum_sleep"],
		max_sleep     = config["maximum_sleep"];

	httplib::Client cli(domain, 80);
	httplib::Headers header = 
	{
		{"Pragma", "no-cache"},
		{"Cache-Control", "no-cache"},
		{"Accept", "*/*"},
		{"DNT", "1"},
		{"User-Agent", user_agent},
		{"Origin", "http://" + domain},
		{"X-Requested-With", "XMLHttpRequest"},
		{"Referer", url},
	};

	// Constructs our multipart form data (with cheats)
	// {"Url":"insert_absolute_path_here"}
	json form_data;
	form_data["Url"] = absolute_path;
	std::string data_encoded = base64_encode(form_data.dump());

	httplib::MultipartFormDataItems data =
	{
		{"data", data_encoded}
	};

	// TODO: Re-add response parser (too lazy, later)
	for (;;)
	{
		auto seed = random_int(0, 100);
		if (seed < like_luckness) {
			auto urlx = std::string("/api/apilike.ashx?func=updatelike&lang=1066&random=" + std::to_string(random_float(0, 1)));
			auto resx = cli.Post(urlx.c_str(), header, data);
			//if (resx->body.find("\"status\":\"SUCCESS\"") != std::string::npos)
			std::cout << "[info] successfully added like" << std::endl;
		} else {
			auto url = std::string("/api/apilike.ashx?func=updateview&lang=1066&random=" + std::to_string(random_float(0, 1)));
			auto res = cli.Post(url.c_str(), header, data);
			//if (res->body.find("\"status\":\"SUCCESS\"") != std::string::npos)
			std::cout << "[info] successfully added view" << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(random_int(min_sleep, max_sleep)));
	}
	
	return 0;
}
