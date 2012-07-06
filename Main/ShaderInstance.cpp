#include "ShaderInstance.hpp"

#include <stdexcept>
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "Preprocessor.hpp"
#include "Factory.hpp"
#include "ShaderSet.hpp"

namespace
{
	std::string convertLang (sh::Language lang)
	{
		if (lang == sh::Language_CG)
			return "SH_CG";
		else if (lang == sh::Language_HLSL)
			return "SH_HLSL";
		else //if (lang == sh::Language_GLSL)
			return "SH_GLSL";
	}

	char getComponent(int num)
	{
		if (num == 0)
			return 'x';
		else if (num == 1)
			return 'y';
		else if (num == 2)
			return 'z';
		else if (num == 3)
			return 'w';
		else
			throw std::runtime_error("invalid component");
	}

	std::string getFloat(sh::Language lang)
	{
		if (lang == sh::Language_CG || lang == sh::Language_HLSL)
			return "float";
		else
			return "vec";
	}
}

namespace sh
{
	std::string Passthrough::expand_assign(std::string toAssign)
	{
		std::string res;

		int i = 0;
		int current_passthrough = passthrough_number;
		int current_component_left = component_start;
		int current_component_right = 0;
		int components_left = num_components;
		while (i < num_components)
		{
			int components_at_once = components_left - current_component_left;
			std::string componentStr;
			for (int j = 0; j < components_at_once; ++j)
				componentStr += getComponent(j + current_component_left);
			std::string componentStr2;
			for (int j = 0; j < components_at_once; ++j)
				componentStr2 += getComponent(j + current_component_right);
			res += "passthrough" + boost::lexical_cast<std::string>(current_passthrough) + "." + componentStr + " = " + toAssign + "." + componentStr2;

			current_component_left += components_at_once;
			current_component_right += components_at_once;
			components_left -= components_at_once;

			i += components_at_once;

			if (components_left == 0)
			{
				// finished
				return res;
			}
			else
			{
				// add semicolon to every instruction but the last
				res += "; ";
			}

			if (current_component_left == 4)
			{
				current_passthrough++;
				current_component_left = 0;
			}
		}
		throw std::runtime_error("expand_assign error"); // this should never happen, but gets us rid of the "control reaches end of non-void function" warning
	}

	std::string Passthrough::expand_receive()
	{
		std::string res;

		res += getFloat(lang) + boost::lexical_cast<std::string>(num_components) + "(";

		int i = 0;
		int current_passthrough = passthrough_number;
		int current_component = component_start;
		int components_left = num_components;
		while (i < num_components)
		{
			int components_at_once = components_left - current_component;
			std::string componentStr;
			for (int j = 0; j < components_at_once; ++j)
				componentStr += getComponent(j + current_component);

			res += "passthrough" + boost::lexical_cast<std::string>(current_passthrough) + "." + componentStr;

			current_component += components_at_once;

			components_left -= components_at_once;

			i += components_at_once;

			if (components_left == 0)
			{
				// finished
				return res + ")";
;
			}
			else
			{
				// add comma to every variable but the last
				res += ", ";
			}

			if (current_component == 4)
			{
				current_passthrough++;
				current_component = 0;
			}
		}

		throw std::runtime_error("expand_receive error"); // this should never happen, but gets us rid of the "control reaches end of non-void function" warning
	}

	// ------------------------------------------------------------------------------

	ShaderInstance::ShaderInstance (ShaderSet* parent, const std::string& name, PropertySetGet* properties)
		: mName(name)
		, mParent(parent)
		, mSupported(true)
		, mCurrentPassthrough(0)
		, mCurrentComponent(0)
	{
		std::string source = mParent->getSource();
		int type = mParent->getType();
		std::string basePath = mParent->getBasePath();

		std::vector<std::string> definitions;

		if (mParent->getType() == GPT_Vertex)
			definitions.push_back("SH_VERTEX_SHADER");
		else
			definitions.push_back("SH_FRAGMENT_SHADER");
		definitions.push_back(convertLang(Factory::getInstance().getCurrentLanguage()));

		// replace properties
		size_t pos;
		while (true)
		{
			pos =  source.find("@shProperty");
			if (pos == std::string::npos)
				break;
			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);
			std::string cmd = source.substr(pos+1, start-(pos+1));
			std::string replaceValue;
			if (cmd == "shPropertyBool")
			{
				std::string propertyName = source.substr(start+1, end-(start+1));
				PropertyValuePtr value = properties->getProperty(propertyName);
				bool val = retrieveValue<BooleanValue>(value, properties->getContext()).get();
				replaceValue = val ? "1" : "0";
			}
			else if (cmd == "shPropertyNotBool") // same as above, but inverts the result
			{
				std::string propertyName = source.substr(start+1, end-(start+1));
				PropertyValuePtr value = properties->getProperty(propertyName);
				bool val = retrieveValue<BooleanValue>(value, properties->getContext()).get();
				replaceValue = val ? "0" : "1";
			}
			else if (cmd == "shPropertyString")
			{
				std::string propertyName = source.substr(start+1, end-(start+1));
				PropertyValuePtr value = properties->getProperty(propertyName);
				replaceValue = retrieveValue<StringValue>(value, properties->getContext()).get();
			}
			else if (cmd == "shPropertyEqual")
			{
				size_t comma_start = source.find(",", pos);
				size_t comma_end = comma_start+1;
				// skip spaces
				while (source[comma_end] == ' ')
					++comma_end;
				std::string propertyName = source.substr(start+1, comma_start-(start+1));
				std::string comparedAgainst = source.substr(comma_end, end-comma_end);
				std::string value = retrieveValue<StringValue>(properties->getProperty(propertyName), properties->getContext()).get();
				replaceValue = (value == comparedAgainst) ? "1" : "0";
			}
			else
				throw std::runtime_error ("unknown command \"" + cmd + "\" in \"" + name + "\"");
			source.replace(pos, (end+1)-pos, replaceValue);
		}

		// replace global settings
		while (true)
		{
			pos =  source.find("@shGlobalSetting");
			if (pos == std::string::npos)
				break;

			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);
			std::string cmd = source.substr(pos+1, start-(pos+1));
			std::string replaceValue;
			if (cmd == "shGlobalSettingBool")
			{
				std::string settingName = source.substr(start+1, end-(start+1));
				std::string value = mParent->getCurrentGlobalSettings()->find(settingName)->second;
				replaceValue = (value == "true" || value == "1") ? "1" : "0";
			}
			if (cmd == "shGlobalSettingEqual")
			{
				size_t comma_start = source.find(",", pos);
				size_t comma_end = comma_start+1;
				// skip spaces
				while (source[comma_end] == ' ')
					++comma_end;
				std::string settingName = source.substr(start+1, comma_start-(start+1));
				std::string comparedAgainst = source.substr(comma_end, end-comma_end);
				std::string value = mParent->getCurrentGlobalSettings()->find(settingName)->second;
				replaceValue = (value == comparedAgainst) ? "1" : "0";
			}

			else
				throw std::runtime_error ("unknown command \"" + cmd + "\" in \"" + name + "\"");
			source.replace(pos, (end+1)-pos, replaceValue);
		}

		// why do we need our own preprocessor? there are several custom commands available in the shader files
		// (for example for binding uniforms to properties or auto constants) - more below. it is important that these
		// commands are _only executed if the specific code path actually "survives" the compilation.
		// thus, we run the code through a preprocessor first to remove the parts that are unused because of
		// unmet #if conditions (or other preprocessor directives).
		Preprocessor p;
		source = p.preprocess(source, basePath, definitions, name);

		// parse foreach
		while (true)
		{
			pos = source.find("@shForeach");
			if (pos == std::string::npos)
				break;

			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);
			int num = boost::lexical_cast<int>(source.substr(start+1, end-(start+1)));

			assert(source.find("@shEndForeach", pos) != std::string::npos);
			size_t block_end = source.find("@shEndForeach", pos);

			// get the content of the inner block
			std::string content = source.substr(end+1, block_end - (end+1));

			// replace both outer and inner block with content of inner block num times
			std::string replaceStr;
			for (int i=0; i<num; ++i)
			{
				// replace @shIterator with the current iteration
				std::string addStr = content;
				boost::replace_all(addStr, "@shIterator", boost::lexical_cast<std::string>(i));
				replaceStr += addStr;
			}
			source.replace(pos, (block_end+std::string("@shEndForeach").length())-pos, replaceStr);
		}

		// parse counter
		std::map<int, int> counters;
		while (true)
		{
			pos = source.find("@shCounter");
			if (pos == std::string::npos)
				break;

			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);

			int index = boost::lexical_cast<int>(source.substr(start+1, end-(start+1)));

			if (counters.find(index) == counters.end())
				counters[index] = 0;

			source.replace(pos, (end+1)-pos, boost::lexical_cast<std::string>(counters[index]++));
		}

		// parse shared parameters
		while (true)
		{
			pos = source.find("@shSharedParameter");
			if (pos == std::string::npos)
				break;

			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);

			mSharedParameters.push_back(source.substr(start+1, end-(start+1)));

			source.erase(pos, (end+1)-pos);
		}

		// parse auto constants
		typedef std::map< std::string, std::pair<std::string, std::string> > AutoConstantMap;
		AutoConstantMap autoConstants;
		while (true)
		{
			pos = source.find("@shAutoConstant");
			if (pos == std::string::npos)
				break;

			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);
			size_t comma1 = source.find(",", pos);
			size_t comma2 = source.find(",", comma1+1);

			bool hasExtraData = (comma2 != std::string::npos) && (comma2 < end);
			std::string autoConstantName, uniformName;
			std::string extraData;
			if (!hasExtraData)
			{
				uniformName = source.substr(start+1, comma1-(start+1));
				// skip spaces
				++comma1;
				while (source[comma1] == ' ')
					++comma1;
				autoConstantName = source.substr(comma1, end-(comma1));
			}
			else
			{
				uniformName = source.substr(start+1, comma1-(start+1));
				// skip spaces
				++comma1;
				while (source[comma1] == ' ')
					++comma1;
				autoConstantName = source.substr(comma1, comma2-(comma1));
				// skip spaces
				++comma2;
				while (source[comma2] == ' ')
					++comma2;
				extraData = source.substr(comma2, end-comma2);
			}
			autoConstants[uniformName] = std::make_pair(autoConstantName, extraData);

			source.erase(pos, (end+1)-pos);
		}

		// parse uniform properties
		while (true)
		{
			pos = source.find("@shUniformProperty");
			if (pos == std::string::npos)
				break;

			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);
			std::string cmd = source.substr(pos, start-pos);
			ValueType vt;
			if (cmd == "@shUniformProperty4f")
				vt = VT_Vector4;
			else if (cmd == "@shUniformProperty3f")
				vt = VT_Vector3;
			else if (cmd == "@shUniformProperty2f")
				vt = VT_Vector2;
			else if (cmd == "@shUniformProperty1f")
				vt = VT_Float;
			else if (cmd == "@shUniformPropertyInt")
				vt = VT_Int;
			else
				throw std::runtime_error ("unsupported command \"" + cmd + "\"");

			size_t comma1 = source.find(",", pos);

			std::string propertyName, uniformName;
			uniformName = source.substr(start+1, comma1-(start+1));
			// skip spaces
			++comma1;
			while (source[comma1] == ' ')
				++comma1;
			propertyName = source.substr(comma1, end-(comma1));
			mUniformProperties[uniformName] = std::make_pair(propertyName, vt);

			source.erase(pos, (end+1)-pos);
		}

		// parse texture samplers used
		while (true)
		{
			pos = source.find("@shUseSampler");
			if (pos == std::string::npos)
				break;

			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);

			mUsedSamplers.push_back(source.substr(start+1, end-(start+1)));
			source.erase(pos, (end+1)-pos);
		}

		// parse passthrough declarations
		while (true)
		{
			pos = source.find("@shAllocatePassthrough");
			if (pos == std::string::npos)
				break;

			if (mCurrentPassthrough > 7)
				throw std::runtime_error ("too many passthrough's requested (max 8)");

			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);
			size_t comma = source.find(",", pos);

			Passthrough passthrough;

			passthrough.num_components = boost::lexical_cast<int>(source.substr(start+1, comma-(start+1)));

			// skip spaces
			++comma;
			while (source[comma] == ' ')
				++comma;

			std::string passthroughName = source.substr(comma, end-comma);
			passthrough.lang = Factory::getInstance().getCurrentLanguage ();
			passthrough.component_start = mCurrentComponent;
			passthrough.passthrough_number = mCurrentPassthrough;

			mPassthroughMap[passthroughName] = passthrough;

			mCurrentComponent += passthrough.num_components;
			if (mCurrentComponent > 3)
				++mCurrentPassthrough;

			source.erase(pos, (end+1)-pos);
		}

		// passthrough assign
		while (true)
		{
			pos = source.find("@shPassthroughAssign");
			if (pos == std::string::npos)
				break;

			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);
			size_t comma = source.find(",", pos);
			std::string passthroughName = source.substr(start+1, comma-(start+1));

			// skip spaces
			++comma;
			while (source[comma] == ' ')
				++comma;

			std::string assignTo = source.substr(comma, end-comma);

			Passthrough& p = mPassthroughMap[passthroughName];

			source.replace(pos, (end+1)-pos, p.expand_assign(assignTo));
		}

		// passthrough receive
		while (true)
		{
			pos = source.find("@shPassthroughReceive");
			if (pos == std::string::npos)
				break;

			size_t start = source.find("(", pos);
			size_t end = source.find(")", pos);
			std::string passthroughName = source.substr(start+1, end-(start+1));

			Passthrough& p = mPassthroughMap[passthroughName];

			source.replace(pos, (end+1)-pos, p.expand_receive());
		}

		// passthrough vertex outputs
		while (true)
		{
			pos = source.find("@shPassthroughVertexOutputs");
			if (pos == std::string::npos)
				break;

			std::string result;
			for (int i = 0; i < mCurrentPassthrough+1; ++i)
			{
				// not using newlines here, otherwise the line numbers reported by compiler would be messed up..
				if (Factory::getInstance().getCurrentLanguage () == Language_CG || Factory::getInstance().getCurrentLanguage () == Language_HLSL)
					result += ", out float4 passthrough" + boost::lexical_cast<std::string>(i) + " : TEXCOORD" + boost::lexical_cast<std::string>(i);
				else
					result += "out vec4 passthrough" + boost::lexical_cast<std::string>(i) + "; ";
			}

			source.replace(pos, std::string("@shPassthroughVertexOutputs").length(), result);
		}

		// passthrough fragment inputs
		while (true)
		{
			pos = source.find("@shPassthroughFragmentInputs");
			if (pos == std::string::npos)
				break;

			std::string result;
			for (int i = 0; i < mCurrentPassthrough+1; ++i)
			{
				// not using newlines here, otherwise the line numbers reported by compiler would be messed up..
				if (Factory::getInstance().getCurrentLanguage () == Language_CG || Factory::getInstance().getCurrentLanguage () == Language_HLSL)
					result += ", in float4 passthrough" + boost::lexical_cast<std::string>(i) + " : TEXCOORD" + boost::lexical_cast<std::string>(i);
				else
					result += "in vec4 passthrough" + boost::lexical_cast<std::string>(i) + "; ";
			}

			source.replace(pos, std::string("@shPassthroughFragmentInputs").length(), result);
		}

		// convert any left-over @'s to #
		boost::algorithm::replace_all(source, "@", "#");

		Platform* platform = Factory::getInstance().getPlatform();

		std::string profile;
		if (Factory::getInstance ().getCurrentLanguage () == Language_CG)
			profile = mParent->getCgProfile ();
		else if (Factory::getInstance ().getCurrentLanguage () == Language_HLSL)
			profile = mParent->getHlslProfile ();


		if (type == GPT_Vertex)
			mProgram = boost::shared_ptr<GpuProgram>(platform->createGpuProgram(GPT_Vertex, "", mName, profile, source, Factory::getInstance().getCurrentLanguage()));
		else if (type == GPT_Fragment)
			mProgram = boost::shared_ptr<GpuProgram>(platform->createGpuProgram(GPT_Fragment, "", mName, profile, source, Factory::getInstance().getCurrentLanguage()));

		//std::cout << source << std::endl;

		if (!mProgram->getSupported())
		{
			std::cerr << "        Full source code below: \n" << source << std::endl;
			mSupported = false;
			return;
		}

		// set auto constants
		for (AutoConstantMap::iterator it = autoConstants.begin(); it != autoConstants.end(); ++it)
		{
			mProgram->setAutoConstant(it->first, it->second.first, it->second.second);
		}

	}

	std::string ShaderInstance::getName ()
	{
		return mName;
	}

	bool ShaderInstance::getSupported () const
	{
		return mSupported;
	}

	std::vector<std::string> ShaderInstance::getUsedSamplers()
	{
		return mUsedSamplers;
	}

	void ShaderInstance::setUniformParameters (boost::shared_ptr<Pass> pass, PropertySetGet* properties)
	{
		for (UniformMap::iterator it = mUniformProperties.begin(); it != mUniformProperties.end(); ++it)
		{
			pass->setGpuConstant(mParent->getType(), it->first, it->second.second, properties->getProperty(it->second.first), properties->getContext());
		}
	}

}
