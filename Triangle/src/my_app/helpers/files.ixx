module;
#include <logging.hxx>

import <filesystem>;
import <fstream>;
import <vector>;

export module Helpers.Files;

namespace Helper {
	export
	std::vector<char> readFile(const std::string &filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
			THROW_Critical ("unable to open file!");

		size_t file_size = size_t(file.tellg());
		std::vector<char> buffer(file_size);
		file.seekg(0);
		
		file.read (buffer.data (), file_size);
		
		file.close ();
		return buffer;
	}
}