#ifndef SH_PREPROCESSOR_H
#define SH_PREPROCESSOR_H

#include <string>
#include <vector>

#include <cstdio>
#include <ostream>
#include <string>
#include <algorithm>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/wave/cpp_throw.hpp>
#include <boost/wave/cpp_exceptions.hpp>
#include <boost/wave/token_ids.hpp>
#include <boost/wave/util/macro_helpers.hpp>
#include <boost/wave/preprocessing_hooks.hpp>

namespace sh
{
	/**
	 * @brief A simple interface for the boost::wave preprocessor
	 */
	class Preprocessor
	{
	public:
		/**
		 * @brief Run a shader source string through the preprocessor
		 * @param source source string
		 * @param includePath path to search for includes (that are included with #include)
		 * @param definitions macros to predefine (vector of strings of the format MACRO=value, or just MACRO to define it as 1)
		 * @param name name to use for error messages
		 * @return processed string
		 */
		static std::string preprocess (std::string source, const std::string& includePath, std::vector<std::string> definitions, const std::string& name);
	};


	class emit_custom_line_directives_hooks
	:   public boost::wave::context_policies::default_preprocessing_hooks
	{
	public:
		///////////////////////////////////////////////////////////////////////////
		//
		//  The function 'emit_line_directive' is called whenever a #line directive
		//  has to be emitted into the generated output.
		//
		//  The parameter 'ctx' is a reference to the context object used for
		//  instantiating the preprocessing iterators by the user.
		//
		//  The parameter 'pending' may be used to push tokens back into the input
		//  stream, which are to be used instead of the default output generated
		//  for the #line directive.
		//
		//  The parameter 'act_token' contains the actual #pragma token, which may
		//  be used for error output. The line number stored in this token can be
		//  used as the line number emitted as part of the #line directive.
		//
		//  If the return value is 'false', a default #line directive is emitted
		//  by the library. A return value of 'true' will inhibit any further
		//  actions, the tokens contained in 'pending' will be copied verbatim
		//  to the output.
		//
		///////////////////////////////////////////////////////////////////////////
		template <typename ContextT, typename ContainerT>
		bool
		emit_line_directive(ContextT const& ctx, ContainerT &pending,
			typename ContextT::token_type const& act_token)
		{
		// emit a #line directive showing the relative filename instead
		typename ContextT::position_type pos = act_token.get_position();
		unsigned int column = 1;

			typedef typename ContextT::token_type result_type;
			using namespace boost::wave;

			pos.set_column(column);
			pending.push_back(result_type(T_POUND, "#", pos));

			pos.set_column(++column);      // account for '#'


			pending.push_back(result_type(T_STRINGLIT, "line ", pos));
			pos.set_column(column+5);                 // account for 'line '


		// 21 is the max required size for a 64 bit integer represented as a
		// string
		char buffer[22];

			sprintf (buffer, "%d", pos.get_line());

			pending.push_back(result_type(T_INTLIT, buffer, pos));
			pos.set_column(column += (unsigned int)strlen(buffer)); // account for <number>

			pending.push_back(result_type(T_GENERATEDNEWLINE, "\n", pos));


			/*

			pending.push_back(result_type(T_SPACE, " ", pos));
			pos.set_column(++column);                 // account for ' '

		std::string file("\"");
		boost::filesystem::path filename(
			boost::wave::util::create_path(ctx.get_current_relative_filename().c_str()));

			using boost::wave::util::impl::escape_lit;
			file += escape_lit(boost::wave::util::native_file_string(filename)) + "\"";

			pending.push_back(result_type(T_STRINGLIT, file.c_str(), pos));
			pos.set_column(column += (unsigned int)file.size());    // account for filename

			*/

			return true;
		}
	};


}

#endif
