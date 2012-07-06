#include "Preprocessor.hpp"

#include "../mcpp/mcpp_lib.h"

#include <cstdio>
#include <iostream>

#include <fstream>

namespace sh
{
	std::string Preprocessor::preprocess (std::string source, const std::string& includePath, std::vector<std::string> definitions, const std::string& name)
	{
		FILE* temp_file;
		temp_file = tmpfile();

		mcpp_use_mem_buffers(1);

		int num_args = 4 + definitions.size()*2;
		char* args[num_args];
		args[0] = "mcpp";
		args[1] = "/tmp/test.shader";
		args[2] = "-I";

		std::vector<char> writable(includePath.size()+1);
		std::copy(includePath.begin(), includePath.end(), writable.begin());
		char* include = &writable[0];

		args[3] = include;

		std::vector<char> vectors[definitions.size()];

		int i=4;
		int cur=0;
		for (std::vector<std::string>::iterator it = definitions.begin(); it != definitions.end(); ++it)
		{
			args[i] = "-D";
			++i;

			std::string val = *it;
			std::vector<char> writable2(val.size()+1);
			std::copy(val.begin(), val.end(), writable2.begin());
			vectors[cur] = writable2;

			args[i] = &vectors[cur][0];
			++cur;
			++i;
		}

		mcpp_lib_main(num_args, args, temp_file);

		char* result = mcpp_get_mem_buffer (OUT);

		return std::string(result);
	}
}
