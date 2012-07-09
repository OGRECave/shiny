#ifndef SH_MATERIALINSTANCE_H
#define SH_MATERIALINSTANCE_H

#include <vector>

#include "PropertyBase.hpp"
#include "Platform.hpp"
#include "MaterialInstancePass.hpp"

namespace sh
{
	class Factory;

	typedef std::vector<MaterialInstancePass> PassVector;

	/**
	 * @brief
	 * A specific material instance, which has all required properties set
	 * (for example the diffuse & normal map, ambient/diffuse/specular values). \n
	 * Depending on these properties, the system will automatically select a shader permutation
	 * that suits these and create the backend materials / passes (provided by the \a Platform class).
	 */
	class MaterialInstance : public PropertySetGet
	{
	public:
		MaterialInstance (const std::string& name, Factory* f);
		virtual ~MaterialInstance ();

		MaterialInstancePass* createPass ();
		PassVector getPasses(); ///< gets the passes of the top-most parent

		/// @attention Because the backend material passes are created on demand, the returned material here might not contain anything yet!
		/// The only place where you should use this method, is for the MaterialInstance given by the MaterialListener::materialCreated event!
		Material* getMaterial();

		void updateShaders ();

		virtual void setProperty (const std::string& name, PropertyValuePtr value);

	private:
		void setParentInstance (const std::string& name);
		std::string getParentInstance ();

		void create (Platform* platform);
		void createForConfiguration (const std::string& configuration);

		void destroyAll ();

		void markDirty (const std::string& configuration); ///< force recreating the technique/shaders when it's next used

		void setShadersEnabled (bool enabled);

		friend class Factory;


	private:
		std::string mParentInstance;
		///< this is only used during the file-loading phase. an instance could be loaded before its parent is loaded,
		/// so initially only the parent's name is written to this member.
		/// once all instances are loaded, the actual mParent pointer (from PropertySetGet class) can be set

		PassVector mPasses;

		std::string mName;

		boost::shared_ptr<Material> mMaterial;

		bool mShadersEnabled;

		Factory* mFactory;
	};
}

#endif
