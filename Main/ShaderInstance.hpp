#ifndef SH_SHADERINSTANCE_H
#define SH_SHADERINSTANCE_H

#include <vector>

#include "Platform.hpp"

namespace sh
{
	class ShaderSet;

	typedef std::map< std::string, std::pair<std::string, ValueType > > UniformMap;

	struct Passthrough
	{
		std::string name;
		int num_components; ///< e.g. 4 for a float4

		std::string expand_to; ///< string to expand to when macro @shPassthrough(type, name) is called
	};

	/**
	 * @brief A specific instance of a \a ShaderSet with a deterministic shader source
	 */
	class ShaderInstance
	{
	public:
		ShaderInstance (ShaderSet* parent, const std::string& name, PropertySetGet* properties);

		std::string getName();

		bool getSupported () const;

		std::vector<std::string> getUsedSamplers();
		std::vector<std::string> getSharedParameters() { return mSharedParameters; }

		void setUniformParameters (boost::shared_ptr<Pass> pass, PropertySetGet* properties);

	private:
		boost::shared_ptr<GpuProgram> mProgram;
		std::string mName;
		ShaderSet* mParent;
		bool mSupported; ///< shader compilation was sucessful?

		std::vector<std::string> mUsedSamplers;
		///< names of the texture samplers that are used by this shader

		std::vector<std::string> mSharedParameters;

		UniformMap mUniformProperties;
		///< uniforms that this depends on, and their property names / value-types
		/// @note this lists shared uniform parameters as well

		int currentPassthrough; ///< 0 - x
		int currentComponent; ///< 0:x, 1:y, 2:z, 3:w

		std::vector<Passthrough> mPassthroughVector;
	};
}

#endif
