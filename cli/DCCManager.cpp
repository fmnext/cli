#include "DCCManager.hpp"

std::vector<std::filesystem::path> references;

bool DCCManager::Init()
{
	printf("Dependencies \n");
	// Prepare the FBX SDK.
	InitializeSdkObjects(mManager);

	mScene = FbxScene::Create(mManager, "root");

	mRootNode = mScene->GetRootNode();

	m_container = fmnext::ContainerReader(mInputPath.string());
	m_game = std::make_unique<fmnext::GameResolver>(mInputPath.string());

	std::vector<char> buffer{};
	if (m_container.findName(std::filesystem::path(mInputPath).filename().replace_extension(std::string(".carbin")).string(), buffer)) {
		auto reader = fmnext::SceneReader(buffer, fmnext::Series::Auto);
		if (reader.Init()) {
			m_scene = std::make_unique<fmnext::SceneReader::Scene>(reader.scene);
		}
	}

	if (m_scene != nullptr)
	{
		auto sqlm = fmnext::SQLManager(m_game->GetDatabase().string(), m_scene->media_name);
		m_records = sqlm.GetQuery();

		if (m_records != nullptr)
		{
			auto tires_directory = m_game->GetSharedTires(m_records->TireModelName);
			auto tires_container = fmnext::ContainerReader(tires_directory.string());
			auto tires = tires_container.getMediaTireEntries();

			for (auto& tire_bundle_name : tires) {
				std::vector<char> tire_buffer{};
				if (tires_container.findName(tire_bundle_name, tire_buffer)) {
					auto reader = fmnext::BundleReader(tire_buffer);
					if (reader.Init()) {
						m_tires.try_emplace(tire_bundle_name, std::make_shared<fmnext::BundleReader::BundleData>(reader.bundle));
					}
				}
			}
		}
		else
		{
			printf("\tRapidJSON %s\n", RAPIDJSON_VERSION_STRING);

			std::filesystem::path fallback_file = std::filesystem::current_path();
			fallback_file /= "fallback.json";
			fallback_file.make_preferred();

			std::ifstream ifs(fallback_file);
			rapidjson::IStreamWrapper isw(ifs);
			if (ifs.is_open())
			{
				rapidjson::Document document{};
				document.ParseStream(isw);

				if (document.HasMember("metadata") && document.HasMember("data"))
				{
					if (document["metadata"].HasMember("type") && document["metadata"]["type"] == "gameDB_Fallback")
					{
						m_records = std::make_shared<fmnext::DataBaseRecords>();

						m_records->MediaName = m_scene->media_name; //document["data"]["Data_Car"]["MediaName"].GetString();
						m_records->CarId = m_scene->ordinal; //document["data"]["Data_Car"]["CarId"].GetInt();
						m_records->CarBodyID = m_scene->CarBodyID; //document["data"]["List_UpgradeCarBody"]["CarBodyID"].GetInt();
						m_records->TireModelName = document["data"]["List_UpgradeTireCompound"]["TireModelName"].GetString();
						m_records->FrontTireWidthMM = document["data"]["Data_Car"]["FrontTireWidthMM"].GetInt();
						m_records->FrontTireAspect = document["data"]["Data_Car"]["FrontTireAspect"].GetInt();
						m_records->FrontWheelDiameterIN = document["data"]["Data_Car"]["FrontWheelDiameterIN"].GetInt();
						m_records->RearTireWidthMM = document["data"]["Data_Car"]["RearTireWidthMM"].GetInt();
						m_records->RearTireAspect = document["data"]["Data_Car"]["RearTireAspect"].GetInt();
						m_records->RearWheelDiameterIN = document["data"]["Data_Car"]["RearWheelDiameterIN"].GetInt();
						m_records->Thumbnail = document["data"]["Data_Car"]["Thumbnail"].GetString();
						m_records->ModelWheelbase = document["data"]["Data_CarBody"]["ModelWheelbase"].GetFloat();
						m_records->ModelFrontTrackOuter = document["data"]["Data_CarBody"]["ModelFrontTrackOuter"].GetFloat();
						m_records->ModelRearTrackOuter = document["data"]["Data_CarBody"]["ModelRearTrackOuter"].GetFloat();
						m_records->ModelFrontStockRideHeight = document["data"]["Data_CarBody"]["ModelFrontStockRideHeight"].GetFloat();
						m_records->ModelRearStockRideHeight = document["data"]["Data_CarBody"]["ModelRearStockRideHeight"].GetFloat();
						m_records->BottomCenterWheelbasePosX = document["data"]["Data_CarBody"]["BottomCenterWheelbasePosX"].GetFloat();
						m_records->BottomCenterWheelbasePosY = document["data"]["Data_CarBody"]["BottomCenterWheelbasePosY"].GetFloat();
						m_records->BottomCenterWheelbasePosZ = document["data"]["Data_CarBody"]["BottomCenterWheelbasePosZ"].GetFloat();

						auto tires_directory = m_game->GetSharedTires(m_records->TireModelName);
						auto tires_container = fmnext::ContainerReader(tires_directory.string());
						auto tires = tires_container.getMediaTireEntries();

						for (auto& tire_bundle_name : tires) {
							std::vector<char> tire_buffer{};
							if (tires_container.findName(tire_bundle_name, tire_buffer)) {
								auto reader = fmnext::BundleReader(tire_buffer);
								if (reader.Init()) {
									m_tires.try_emplace(tire_bundle_name, std::make_shared<fmnext::BundleReader::BundleData>(reader.bundle));
								}
							}
						}
					}
				}
			}
		}

		{
			std::vector<char> buffer{};
			if (m_container.findName("BuildNumber.txt", buffer)) {
				build_number = GetBuildNumber(buffer);
			}
		}

		printf("\tlibzip %s\n", zip_libzip_version());
		printf("\tGranny %s \n", GrannyGetVersionString());

		FBXSDK_printf("\n");
		FBXSDK_printf("Model Scene\n");

		std::cout << "\tMedia Name: " << m_scene->media_name << "\n";
		std::cout << "\tGUID: " << GetStringGUID(m_scene->build_guid) << "\n";
		std::cout << "\tID: " << m_scene->ordinal << "\n";
		std::cout << "\tSkeleton: " << m_scene->skeleton_modelbin_path << "\n";
		std::cout << "\tSeries: " << m_game->GetSeriesName() << "\n";
		std::cout << "\tBuild Number: " << build_number << "\n";

		std::string lApplicationName = "ForzaTech CLI Toolkit v";
		lApplicationName += GetVersionString();

		// create scene info
		FbxDocumentInfo* sceneInfo = FbxDocumentInfo::Create(mManager, "SceneInfo");
		sceneInfo->mTitle = m_scene->media_name.c_str();
		sceneInfo->mSubject = build_number.c_str();
		sceneInfo->mAuthor = lApplicationName.c_str();
		sceneInfo->mRevision = "rev. 1.0";
		sceneInfo->mKeywords = "forza scene";
		sceneInfo->mComment = "no particular comments required.";

		sceneInfo->Original_ApplicationVersion = GetVersionString().c_str();
		sceneInfo->Original_ApplicationName = "ForzaTech CLI Toolkit";
		sceneInfo->Original_ApplicationVendor = "Apex";

		// we need to add the sceneInfo before calling AddThumbNailToScene because
		// that function is asking the scene for the sceneInfo.
		mScene->SetSceneInfo(sceneInfo);
		mScene->GetGlobalSettings().SetTimeMode(FbxTime::eFrames60);

		if (GrannyVersionsMatch && m_CharacterFile == nullptr) {
			std::string skeleton_location = "scene/";
			skeleton_location += m_scene->media_name;
			skeleton_location += "_skeleton.gr2";

			std::vector<char> gr2_buffer{};
			if (m_container.findName(skeleton_location, gr2_buffer)) {
				SetSkeleton(gr2_buffer);
			}
		}
		else {
			GetSkeleton();
		}

		FBXSDK_printf("\n");
		FBXSDK_printf("Animations\n");

		if (GrannyVersionsMatch)
		{
			std::string state_machine = "animations/";
			state_machine += m_scene->media_name;
			state_machine += ".gsf";

			std::vector<char> gsf_buffer{};
			if (m_container.findName(state_machine, gsf_buffer))
			{
				InitStateMachine(gsf_buffer);

				granny_file_info* GrannyModelInfo = GrannyGetFileInfo(m_CharacterFile);

				for (auto& ref : references)
				{
					std::string motion_path = "game:\\media\\cars\\";
					motion_path += m_scene->media_name;
					motion_path += "\\animations\\";
					motion_path += std::filesystem::path(ref.string()).filename().string();

					std::cout << "\t" << motion_path << "\n";

					std::vector<char> buffer{};
					if (m_container.findName(fmnext::GameResolver::RemoveBase(motion_path, m_scene->media_name), buffer)) {
						granny_file* GrannyFileAnim = GrannyReadEntireFileFromMemory(static_cast<granny_int32x>(buffer.size()), buffer.data());
						granny_file_info* GrannyAnimInfo = GrannyGetFileInfo(GrannyFileAnim);

						HandleAnimation(GrannyModelInfo->Skeletons[0], GrannyAnimInfo->Animations[0], std::filesystem::path(ref.string()).stem().string());
						GrannyFreeFile(GrannyFileAnim);
					}

				}
			}
		}
		else
		{
			printf("Warning: the Granny DLL currently loaded "
				"doesn't match the .h file used during compilation\n");
		}


	}

	SetSkeleton(m_scene->skeleton_modelbin_path);

	if (m_CharacterFile == nullptr)
	{
		HandleSkeleton(m_skel->Skeleton);
	}

	{
		FBXSDK_printf("\n");
		FBXSDK_printf("Proxy LOD\n");

		std::string proxyLOD = MakeCarRelativePath("Scene/ProxyLOD.modelbin");
		SetProxyLOD(proxyLOD);

		if (m_proxyLOD != nullptr)
		{
			HandleProxyLOD();
			FBXSDK_printf("\t%s\n", proxyLOD.c_str());
		}
		else {
			FBXSDK_printf("\tNone\n");
		}
	}

	FBXSDK_printf("\n");
	FBXSDK_printf("Materials\n");
	FBXSDK_printf("\tProcessing...\n");

	// media/_library and media/cars/_library
	if (!m_game->GetBase().empty())
	{
		materials_container.push_back(fmnext::ContainerReader(m_game->GetPrimaryMaterialsLibrary().string()));
		materials_container.push_back(fmnext::ContainerReader(m_game->GetMediaMaterialsLibrary().string()));
	}

	if (!m_game->GetBase().empty())
	{
		textures_container.push_back(fmnext::ContainerReader(m_game->GetPrimaryTexturesLibrary().string()));
		textures_container.push_back(fmnext::ContainerReader(m_game->GetMediaTexturesLibrary().string()));
	}

	// media/cars/_library
	if (!m_game->GetBase().empty())
	{
		auto sec_mat = m_game->GetSecondaryMaterialsLibrary();
		auto sec_tex = m_game->GetSecondaryTexturesLibrary();

		for (auto& mat_lib_entry : sec_mat)
		{
			std::string message;
			message += "Secondary Material Library Found: ";
			message += mat_lib_entry.string();
			message += " \n";

			materials_container.push_back(fmnext::ContainerReader(mat_lib_entry.string()));

			//MGlobal::displayInfo(message.c_str());
		}

		for (auto& tex_lib_entry : sec_tex)
		{
			std::string message;
			message += "Secondary Texture Library Found: ";
			message += tex_lib_entry.string();
			message += " \n";

			textures_container.push_back(fmnext::ContainerReader(tex_lib_entry.string()));

			//MGlobal::displayInfo(message.c_str());
		}
	}


	for (const auto& upgradable_part : m_scene->upgradable_parts)
	{
		for (const auto& upgrade : upgradable_part.upgrade_models)
		{
			if (upgrade.car_body_id == -1)
				continue;

			if (auto item = std::find_if(car_bodies.begin(), car_bodies.end(), [&](auto& car_body) { return car_body.first == upgrade.id; }); item != std::end(car_bodies))
			{
				if (car_bodies[upgrade.car_body_id] != upgrade.parent_is_stock)
				{
					printf("Warning: CarBody %i is marked as both stock and non-stock. \n", upgrade.car_body_id);
				}
			}
			else
			{
				car_bodies.emplace(upgrade.car_body_id, upgrade.parent_is_stock);
			}
		}
	}

	if (m_records)
	{
		car_upgrades.try_emplace(m_records->CarBodyID, true);
	}

	for (const auto& [id, parent_istock] : car_bodies) {
		if (parent_istock) {
			car_upgrades.try_emplace(id, true);
		}
	}


	for (const auto& upgradable_part : m_scene->upgradable_parts)
	{
		for (const auto& upgrade : upgradable_part.upgrade_models)
		{
			if (auto item = std::find_if(car_upgrades.begin(), car_upgrades.end(), [&](auto& car_upgrade) { return car_upgrade.first == upgrade.id; }); item == std::end(car_upgrades))
			{
				if (auto result = std::find_if(std::begin(upgradable_part.shared_models), std::end(upgradable_part.shared_models), [&](const auto& data) { for (auto& upgrade_id : data.upgrade_ids) { return upgrade_id == upgrade.id; } return false;  }); result != std::end(upgradable_part.shared_models))
				{
					car_upgrades.emplace(upgrade.id, upgrade.is_stock);
				}
			}
		}
	}



	for (const auto& part : m_scene->upgradable_parts)
	{
		for (auto& [upgrade_ids, model] : part.shared_models)
		{
			for (auto& id : upgrade_ids)
			{
				if (auto upgrade_item = std::find_if(std::begin(part.upgrade_models), std::end(part.upgrade_models), [&](const auto& data) { return data.id == id; }); upgrade_item != std::end(part.upgrade_models))
				{
					//QString scheme = QString("%0/%1/%2").arg(std::to_string(id).c_str(), root_item->data(0, Qt::DisplayRole).toString(), model->type.c_str());

					auto bundle = SetBundleData(model);

					if (bundle) {
						std::string scheme = "";
						auto materials = HandleShaders(model, bundle, scheme);

						list_items.emplace_back(upgrade_item->id, model, bundle, materials, scheme, static_cast<uint32_t>(part.type));
					}

				}
			}
		}

	}

	for (const auto& part : m_scene->non_upgradable_parts)
	{
		uint32_t upgrade_id = (m_records == nullptr) ? 0 : m_records->CarBodyID;
		for (const auto& [id, parent_istock] : car_bodies) {
			if (parent_istock) {
				upgrade_id = id;
			}
		}

		if (auto stock = std::find_if(car_upgrades.begin(), car_upgrades.end(), [&](auto val) { return val.second == true; }); stock != car_upgrades.end())
		{
			upgrade_id = stock->first;
		}

		for (const auto& model : part.models)
		{
			auto bundle = SetBundleData(model);

			if (bundle) {
				std::string scheme = "";
				auto materials = HandleShaders(model, bundle, scheme);

				list_items.emplace_back(upgrade_id, model, bundle, materials, scheme, static_cast<uint32_t>(part.type));
			}

		}

		if (part.type == fmnext::CCarParts_WheelStyle && m_records && !m_tires.empty())
		{
			for (const auto& model : part.models)
			{
				std::shared_ptr<fmnext::SceneReader::CarRenderModel11> tire_model = std::make_shared<fmnext::SceneReader::CarRenderModel11>();
				tire_model->bone_id = model->bone_id;
				tire_model->bone_name = model->bone_name;
				tire_model->id = model->id;
				tire_model->levels_of_detail = model->levels_of_detail;
				tire_model->draw_groups = model->draw_groups;
				tire_model->transform = model->transform;
				tire_model->version = model->version;
				tire_model->type = "Tires";

				std::string position("tire");
				position += GetContainerDirection(model->bone_name);

				//{ "tireL", "tireL", "tireL", "tireL" };     =  previous
				//{ "tireL", "tireL", "tireR", "tireR" };     =  previous/current
				//{ "tireLF", "tireLR", "tireRF", "tireRR" }; =  current/fixed

				for (const auto& [key, data] : m_tires)
				{
					std::smatch match_tire{};
					std::string regex = (m_tires.size() <= 2) ? std::string(position.begin(), position.end() - 1) : position;

					switch (static_cast<uint32_t>(m_tires.size()))
					{
					case 0x01:
					{
						tire_model->path = "game:\\media\\cars\\_library\\scene\\tires\\";
						tire_model->path += m_records->TireModelName;
						tire_model->path += std::string(std::string("\\") + position + std::string(key.begin() + regex.size(), key.end()));

						if (std::find_if(list_items.begin(), list_items.end(), [&](const auto& pitem) { return pitem.model->path == tire_model->path; }) == std::end(list_items))
						{
							std::string scheme = "";
							auto materials = HandleShaders(tire_model, data, scheme);

							list_items.emplace_back(upgrade_id, tire_model, data, materials, scheme, 8);
						}
						break;
					}
					case 0x02:
					case 0x04:
					{
						if (std::regex_search(key, match_tire, std::regex(regex, std::regex::icase)))
						{
							tire_model->path = "game:\\media\\cars\\_library\\scene\\tires\\";
							tire_model->path += m_records->TireModelName;
							tire_model->path += std::string(std::string("\\") + position + std::string(key.begin() + regex.size(), key.end()));

							if (std::find_if(list_items.begin(), list_items.end(), [&](const auto& pitem) { return pitem.model->path == tire_model->path; }) == std::end(list_items))
							{
								std::string scheme = "";
								auto materials = HandleShaders(tire_model, data, scheme);

								list_items.emplace_back(upgrade_id, tire_model, data, materials, scheme, 8);
							}
						}
						break;
					}
					default:
						printf("Warning: Unsupported number of tire entries found: 0x%02X \n", static_cast<uint32_t>(m_tires.size()));
						break;
					}
				}
			}
		}

	}

	SetupOutpuDirectory();
	SetupOutputTextures();

	FBXSDK_printf("\n");
	std::cout << "Thumbnail" << "\n";
	if (m_records)
	{
		if (!m_records->Thumbnail.empty())
		{
			for (const auto& path : m_game->GetResourceContainer(m_records->Thumbnail))
			{
				for (const auto& name : fmnext::GameResolver::GetThumbnailNames(m_records->Thumbnail))
				{
					auto thumbnail_container = fmnext::ContainerReader(path.string());

					std::vector<char> thumb_blob{};
					if (thumbnail_container.findName(name, thumb_blob)) {
						auto thumb = fmnext::BundleReader(thumb_blob);
						if (thumb.Init())
						{
							auto l_thumbnail = std::make_unique<fmnext::BundleReader::BundleData>(thumb.bundle);

							std::string lfilename(std::filesystem::path(name).stem().string());
							lfilename += "_";
							lfilename += path.stem().string();
							lfilename += ".png";

							ExportThumbnail(std::move(l_thumbnail), lfilename);
						}

						std::cout << "\t" << name << "\n";
						std::cout << "\t" << m_records->Thumbnail << "\n";
						std::cout << "\t" << path.string() << "\n";
					}

					std::cout << "\n";
				}
			}
		}
	}

	if (m_records)
	{
		if (m_records->Thumbnail.empty())
		{
			std::cout << "\t" << "Not available in fallback." << "\n";
		}
	}

	FBXSDK_printf("\n");
	FBXSDK_printf("Textures\n");

	if (true) // ?? arg --textures 1 ????? ?? 
	{
		for (auto& [path, filename] : texture_list)
		{
			auto library_blob = HandleLibrary(path);

			if (!library_blob.empty())
			{
				auto texture_bundle = fmnext::BundleReader(library_blob);
				if (texture_bundle.Init())
				{
					std::filesystem::path path_name(mTextureOutputPath);
					path_name /= std::filesystem::path(filename).replace_extension(".dds").string();
					path_name.make_preferred();

					auto texture_resolver = fmnext::TextureResolver(texture_bundle.bundle);
					texture_resolver.SaveToDDSFile(path_name.string());

					std::cout << "\t" << path << "\n";
				}

			}
		}


		auto Textures = m_container.getMediaTextureEntries();

		for (auto& [path, filename] : Textures)
		{
			std::vector<char> buffer{};
			if (m_container.findName(path, buffer))
			{
				std::smatch match_result;
				std::regex_search(path, match_result, std::regex("UI/Textures", std::regex_constants::icase));

				if (!match_result.ready())
				{
					std::filesystem::path path_name(mTextureOutputPath);

					path_name /= std::filesystem::path(filename).replace_extension(".dds").string();
					path_name.make_preferred();

					if (!std::filesystem::exists(path_name))
					{
						auto texture_bundle = fmnext::BundleReader(buffer);
						if (texture_bundle.Init())
						{
							auto texture_resolver = fmnext::TextureResolver(texture_bundle.bundle);
							texture_resolver.SaveToDDSFile(path_name.string());

							std::cout << "\t" << MakeCarRelativePath(path) << "\n";
						}
					}
				}
				else {
					std::filesystem::path path_name(mTextureOutputPath);

					path_name /= std::filesystem::path(filename).replace_extension(".dds").string();
					path_name.make_preferred();

					auto texture_bundle = fmnext::BundleReader(buffer);
					if (texture_bundle.Init())
					{
						auto texture_resolver = fmnext::TextureResolver(texture_bundle.bundle);
						texture_resolver.SaveToDDSFile(path_name.string());
						//texture_resolver.SaveToPNGFile(path_name.string());

						std::cout << "\t" << MakeCarRelativePath(path) << "\n";
					}
				}
			}

			buffer.clear();
		}
	}

	FBXSDK_printf("\n");
	FBXSDK_printf("Digital Gauges\n");
	{
		std::string digitalGauge = "UI/";
		digitalGauge += m_scene->media_name;
		digitalGauge += ".xaml";

		std::vector<char> buffer{};
		if (m_container.findName(digitalGauge, buffer))
		{
			std::filesystem::path path_name(mOutputPath);
			path_name /= std::filesystem::path(digitalGauge).filename().string();
			path_name.make_preferred();

			if (!std::filesystem::exists(path_name))
			{
				std::ofstream stream(path_name, std::ios::binary | std::ios::out);
				if (stream.is_open())
				{
					stream.write(buffer.data(), buffer.size());
					stream.close();

					std::cout << "\t" << MakeCarRelativePath(digitalGauge) << "\n";
				}
			}

			buffer.clear();
		}
		else {
			std::cout << "\tNone\n";
		}
	}


	FBXSDK_printf("\n");
	FBXSDK_printf("Vehicle Upgrades\n");

	for (auto& [upgrade, is_stock] : car_upgrades)
	{
		std::cout << "\t" << upgrade << "\n";
	}

	FBXSDK_printf("\n");
	FBXSDK_printf("Manufacturer Colors\n");

	m_colors = GetBundleData("ManufacturerColors.bin");

	ExportManufacturerColors();

	if (m_colors != nullptr) //!m_colors->ManufacturerColors.empty()
	{
		if (!m_colors->ManufacturerColors.empty())
		{
			int color_index = 0;

			for (auto& color : m_colors->ManufacturerColors)
			{
				for (auto& inst : color)
				{
					// std::filesystem::path(inst.path).filename().stem().string()
					// inst.path
					// inst.preview_color.x, inst.preview_color.y, inst.preview_color.z

					uint32_t RR = static_cast<uint32_t>(std::round(inst.preview_color.x * 255.0f));
					uint32_t GG = static_cast<uint32_t>(std::round(inst.preview_color.y * 255.0f));
					uint32_t BB = static_cast<uint32_t>(std::round(inst.preview_color.z * 255.0f));

					std::cout << "\t";
					std::cout << " #" << std::uppercase << std::hex << GetHexColor(RR, GG, BB) << "\n";

					{
						if (fmnext::GameResolver::Contains(inst.path, m_scene->media_name))
						{
							std::string path = fmnext::GameResolver::Remove(inst.path, m_scene->media_name).string();
							std::replace(path.begin(), path.end(), '\\', '/');

							std::vector<char> mcolor_blob{};
							if (m_container.findName(path, mcolor_blob))
							{
								auto material = GetBundleData(mcolor_blob);
								//auto material_shader = GetBundleData(material->MaterialInstances[0]);

								for (auto& param : material->ShaderParameters)
								{
									if (param.type == fmnext::ShaderParameter_Texture2D)
									{
										std::string texture = std::any_cast<std::string>(param.value);
										// std::filesystem::path(texture).filename().stem().string()
									}

								}

							}
						}
						else
						{
							std::string path = m_game->Remove(inst.path).string();
							std::replace(path.begin(), path.end(), '\\', '/');

							std::vector<char> mcolor_blob = FindAssetInContainer(path);
							if (!mcolor_blob.empty())
							{
								auto material = GetBundleData(mcolor_blob);
								//auto material_shader = GetBundleData(material->MaterialInstances[0]);

								for (auto& param : material->ShaderParameters)
								{
									if (param.type == fmnext::ShaderParameter_Texture2D)
									{
										std::string texture = std::any_cast<std::string>(param.value);
										// std::filesystem::path(texture).filename().stem().string()
									}

								}
							}
						}


					}
				}

				++color_index;
			}

		}
	}
	else {
		FBXSDK_printf("\tNot available.\n");
	}

	//FbxAxisSystem SceneAxisSystem = mScene->GetGlobalSettings().GetAxisSystem();

	//FbxAxisSystem AxisSystem(FbxAxisSystem::eMax);
	//FbxSystemUnit UnitSystem(FbxSystemUnit::mm);

	//AxisSystem.ConvertScene(mScene);
	//UnitSystem.ConvertScene(mScene);


	//FbxAxisSystem::Max.DeepConvertScene(mScene);

	FBXSDK_printf("\n");
	//FBXSDK_printf("Meshes\n");

	if (m_records)
	{
		Initialize(m_records);
	}
	else {
		Initialize(nullptr);
	}

	FBXSDK_printf("\n");

	std::filesystem::path lOutputPath(mOutputPath);

	std::string lmedianame = std::filesystem::path(mInputPath).stem().string();
	lmedianame += "_";
	lmedianame += lodstr[m_lod];
	lmedianame += ".fbx";

	//lOutputPath /= std::filesystem::path(mInputPath).filename().replace_extension(".fbx");
	lOutputPath /= lmedianame;
	lOutputPath.make_preferred();

	bool lResult = SaveDocument(mManager, mDocument, lOutputPath.string().c_str());

	if (!lResult) FBXSDK_printf("\n\nAn error occurred while saving the document...\n");

	FBXSDK_printf("Releasing resources...\n");

	references.clear();

	// Destroy all objects created by the FBX SDK.
	DestroySdkObjects(mManager, lResult);

	return true;
}

std::string DCCManager::GetVersionString()
{
	std::string version = std::to_string(FT_TOOLKIT_MAJOR_VERSION);
	version += ".";
	version += std::to_string(FT_TOOLKIT_MINOR_VERSION);
	version += ".";
	version += std::to_string(FT_TOOLKIT_PATCH_VERSION);
	version += ".";
	version += std::to_string(FT_TOOLKIT_BUILD_NUMBER);

	return version;
}

void DCCManager::Initialize(std::shared_ptr<fmnext::DataBaseRecords> p_records)
{
	std::unordered_map<std::string, DirectX::XMMATRIX> suspension_transforms;
	std::unordered_map<std::string, FbxNode*> suspensions;

	std::unordered_map<std::string, float> rotor_center_offsets;
	std::unordered_map<std::string, float> spindle_offsets;

	std::unordered_map<std::string, DirectX::XMMATRIX> rotor_transforms;
	std::unordered_map<std::string, FbxNode*> rotors;

	std::unordered_map<std::string, DirectX::XMMATRIX> caliper_transforms;
	std::unordered_map<std::string, FbxNode*> calipers;

	std::unordered_map<std::string, DirectX::XMMATRIX> spindle_transforms;

	int color_override = 0, upgrade_level = 0;

	if (auto stock = std::find_if(car_upgrades.begin(), car_upgrades.end(), [&](auto val) { return val.second == true; }); stock != car_upgrades.end())
	{
		upgrade_level = stock->first;
	}

	if (car_upgrades.empty()) // if somehow CarBodyID is completely missing, this will assume the ID is zero.
	{
		car_upgrades.try_emplace(0, true);
	}

	//if (!DCCManager::objExists(lod_group_name))
	{
		for (auto& [upgrade_id, is_stock] : car_upgrades)
		{
			FbxNode* lodGroupObj = CreateLocator();

			int default_upgrade = upgrade_id == 0 ? m_scene->ordinal : upgrade_id;

			std::string lod_group_name = lodstr[m_lod];
			lod_group_name += "_";
			lod_group_name += std::to_string(default_upgrade);

			lodGroupObj->SetName(lod_group_name.c_str());

			std::cout << std::to_string(default_upgrade) << "\n";

			for (auto& data : list_items)
			{
				if (data.upgrade_id == upgrade_id)
				{
					std::cout << "\t" << data.model->path << "\n";

					if (data.type == 44) // Wheels
					{
						fmnext::PartLocation direction = DCCManager::GetPartDirection(data.model->bone_name);

						if (p_records)
						{
							float ModelTrackOuter{};
							float ModelStockRideHeight{};

							if (direction.end == fmnext::PartLocation::FRONT)
							{
								ModelTrackOuter = p_records->ModelFrontTrackOuter;
								ModelStockRideHeight = p_records->ModelFrontStockRideHeight;
							}

							if (direction.end == fmnext::PartLocation::MID)
							{
								ModelTrackOuter = p_records->ModelFrontTrackOuter;
								ModelStockRideHeight = p_records->ModelFrontStockRideHeight;
							}

							if (direction.end == fmnext::PartLocation::REAR)
							{
								ModelTrackOuter = p_records->ModelRearTrackOuter;
								ModelStockRideHeight = p_records->ModelRearStockRideHeight;
							}

							int TireWidthMM = -1;
							int TireAspect = -1;
							int WheelOriginalDiameterIN = -1;
							int WheelDiameterIN = -1;

							switch (direction.end)
							{
							case fmnext::PartLocation::FRONT:
								TireWidthMM = p_records->FrontTireWidthMM;
								TireAspect = p_records->FrontTireAspect;
								WheelOriginalDiameterIN = p_records->FrontWheelDiameterIN;
								WheelDiameterIN = p_records->FrontWheelDiameterIN;
								break;
							case fmnext::PartLocation::MID:
								TireWidthMM = p_records->FrontTireWidthMM;
								TireAspect = p_records->FrontTireAspect;
								WheelOriginalDiameterIN = p_records->FrontWheelDiameterIN;
								WheelDiameterIN = p_records->FrontWheelDiameterIN;
								break;
							case fmnext::PartLocation::REAR:
								TireWidthMM = p_records->RearTireWidthMM;
								TireAspect = p_records->RearTireAspect;
								WheelOriginalDiameterIN = p_records->RearWheelDiameterIN;
								WheelDiameterIN = p_records->RearWheelDiameterIN;
								break;
							default:
								// unreachable
								break;
							}

							float half_wheel_outer_diameter_m = static_cast<float>(((TireAspect * 0.01) * (TireWidthMM * 0.001)) + WheelDiameterIN * 0.0254 / 2);

							DirectX::XMFLOAT4 translate_v1 = DirectX::XMFLOAT4((ModelTrackOuter / 2), (half_wheel_outer_diameter_m - ModelStockRideHeight), (p_records->ModelWheelbase / 2), 0);
							// y = or (min + max) / 2

							if (direction.side == fmnext::PartLocation::LEFT)
							{
								translate_v1.x = -translate_v1.x;
							}

							if (direction.end == fmnext::PartLocation::REAR)
							{
								translate_v1.z = -translate_v1.z;
							}

							translate_v1.x += p_records->BottomCenterWheelbasePosX;
							translate_v1.y += p_records->BottomCenterWheelbasePosY;
							translate_v1.z -= p_records->BottomCenterWheelbasePosZ;

							if (direction.end == fmnext::PartLocation::MID && direction.side == fmnext::PartLocation::LEFT)
							{
								//translate_v1.z = -translate_v1.z;

								DirectX::XMVECTOR outScale, outQuat, outTrans;
								DirectX::XMMatrixDecompose(&outScale, &outQuat, &outTrans, data.model->transform);

								translate_v1.z = DirectX::XMVectorGetZ(outTrans);
							}

							if (direction.end == fmnext::PartLocation::MID && direction.side == fmnext::PartLocation::RIGHT)
							{
								//translate_v1.z = -translate_v1.z;

								DirectX::XMVECTOR outScale, outQuat, outTrans;
								DirectX::XMMatrixDecompose(&outScale, &outQuat, &outTrans, data.model->transform);

								translate_v1.z = DirectX::XMVectorGetZ(outTrans);
							}

							DirectX::XMMATRIX spidle_transform = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat4(&translate_v1));

							if (direction.side == fmnext::PartLocation::RIGHT)
							{
								DirectX::XMVECTOR v1(spidle_transform.r[0]);
								DirectX::XMVECTOR v2(spidle_transform.r[2]);

								spidle_transform.r[0] = DirectX::XMVectorSet(-DirectX::XMVectorGetX(v1), DirectX::XMVectorGetY(v1), DirectX::XMVectorGetZ(v1), DirectX::XMVectorGetW(v1));
								spidle_transform.r[2] = DirectX::XMVectorSet(DirectX::XMVectorGetX(v2), DirectX::XMVectorGetY(v2), -DirectX::XMVectorGetZ(v2), DirectX::XMVectorGetW(v2));
							}

							spindle_transforms.emplace(data.model->bone_name, spidle_transform);

							{
								float spindle_offset{};
								float control_arm_offset = 0.30480003f; // 12 inch(0x3E9C0EC0)

								std::string boneName = "spindle";

								for (auto& bone : data.bundle->Skeleton)
								{
									if (bone.name == boneName)
									{
										spindle_offset = DirectX::XMVectorGetX(bone.transform.r[3]);
										break;
									}
								}

								for (auto& bone : m_skel->Skeleton)
								{
									if (bone.name == "controlArm")
									{
										control_arm_offset = DirectX::XMVectorGetX(bone.transform.r[3]);
										break;
									}
								}

								if (auto rotor_data = std::find_if(std::begin(list_items), std::end(list_items), [&](auto& mdl) { return mdl.model->bone_name == data.model->bone_name && mdl.model->type == "Brakes"; });
									rotor_data != std::end(list_items)) {
									for (auto& bone : rotor_data->bundle->Skeleton)
									{
										if (bone.name == "controlArm")
										{
											control_arm_offset = DirectX::XMVectorGetX(bone.transform.r[3]);
											break;
										}
									};
								}

								spindle_offsets.emplace(data.model->bone_name, spindle_offset);


								DirectX::XMFLOAT4 translate_v2 = DirectX::XMFLOAT4(spindle_offset, 0.f, 0.f, 1.f);

								translate_v2.x += control_arm_offset;

								if (direction.side == fmnext::PartLocation::RIGHT)
								{
									translate_v2.x = -translate_v2.x;
								}

								translate_v2.x += DirectX::XMVectorGetX(spidle_transform.r[3]);
								translate_v2.y += DirectX::XMVectorGetY(spidle_transform.r[3]);
								translate_v2.z += DirectX::XMVectorGetZ(spidle_transform.r[3]);
								translate_v2.w = 1.f;

								std::string suspension_name = "controlArm_" + DCCManager::GetContainerDirection(data.model->bone_name);

								DirectX::XMMATRIX controlArm_transform = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat4(&translate_v2));

								suspension_transforms.emplace(suspension_name, controlArm_transform);

								// wheels
								auto resolver = fmnext::MeshResolver(data.bundle, m_lod, static_cast<fmnext::GeometryType>(m_geo), p_records, data.type, direction.end);

								std::string wheel_name = "wheel_" + DCCManager::GetContainerDirection(data.model->bone_name);

								FbxNode* locatorObj = nullptr;

								if (!resolver.GetMeshes().empty())
								{
									locatorObj = CreateLocator(spidle_transform);

									//fnDagNode.setObject(wheelLocatorObj);
									locatorObj->SetName(wheel_name.c_str());
								}

								for (auto& mesh : resolver.GetMeshes())
								{
									FbxNode* mesh_obj = nullptr;
									FbxSurfaceLambert* material_obj = nullptr;

									std::string mesh_name{};

									auto material = std::find_if(data.bundle->MaterialInstanceBundles.begin(), data.bundle->MaterialInstanceBundles.end(), [&](auto& mtl) {
										return std::any_cast<int32_t>(mtl.metadata["Id"]) == mesh.material_index;
										});

									if (material != std::end(data.bundle->MaterialInstanceBundles))
									{
										mesh_name += mesh.name;
										mesh_name += "_";
										mesh_name += std::any_cast<std::string>(material->metadata["Name"]);
									}


									if (auto material_it = data.materials.find(mesh.material_index); material_it != std::end(data.materials))
									{
										const auto& [key, material_data] = *material_it;

										material_obj = CreateMaterialfromMemory(std::any_cast<std::string>(material->metadata["Name"]), material_data.instace);
									}

									mesh_obj = CreateMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs, mesh_name, material_obj, static_cast<fmnext::GeometryType>(m_geo));
									SetNodeTransformation(mesh_obj, mesh.matrix);

									locatorObj->AddChild(mesh_obj);
								}


								lodGroupObj->AddChild(locatorObj);
							}
						}

						// wheels
						if (!p_records)
						{
							auto resolver = fmnext::MeshResolver(data.bundle, m_lod, static_cast<fmnext::GeometryType>(m_geo), nullptr, data.type, direction.end);

							std::string wheel_name = "wheel_" + DCCManager::GetContainerDirection(data.model->bone_name);

							FbxNode* locatorObj = nullptr;

							if (!resolver.GetMeshes().empty())
							{
								locatorObj = CreateLocator(data.model->transform);
								locatorObj->SetName(wheel_name.c_str());
							}

							for (auto& mesh : resolver.GetMeshes())
							{
								FbxNode* mesh_obj = nullptr;
								FbxSurfaceLambert* material_obj = nullptr;

								std::string mesh_name{};

								auto material = std::find_if(data.bundle->MaterialInstanceBundles.begin(), data.bundle->MaterialInstanceBundles.end(), [&](auto& mtl) {
									return std::any_cast<int32_t>(mtl.metadata["Id"]) == mesh.material_index;
									});

								if (material != std::end(data.bundle->MaterialInstanceBundles))
								{
									mesh_name += mesh.name;
									mesh_name += "_";
									mesh_name += std::any_cast<std::string>(material->metadata["Name"]);
								}

								if (auto material_it = data.materials.find(mesh.material_index); material_it != std::end(data.materials))
								{
									const auto& [key, material_data] = *material_it;

									material_obj = CreateMaterialfromMemory(std::any_cast<std::string>(material->metadata["Name"]), material_data.instace);
								}

								mesh_obj = CreateMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs, mesh_name, material_obj, static_cast<fmnext::GeometryType>(m_geo));
								SetNodeTransformation(mesh_obj, mesh.matrix);

								locatorObj->AddChild(mesh_obj);
							}

							lodGroupObj->AddChild(locatorObj);
						}

						continue;
					}

					if (data.type == 8) // Tires
					{
						if ((m_lod == 0) && data.model->levels_of_detail.LODS || (m_lod >= 1) && !data.model->levels_of_detail.LODS || (m_lod >= 0) && data.model->levels_of_detail.LODS)
						{
							fmnext::PartLocation direction = DCCManager::GetPartDirection(data.model->bone_name);

							auto resolver = fmnext::MeshResolver(data.bundle, m_lod, static_cast<fmnext::GeometryType>(m_geo), p_records, 8, direction.end);

							std::string tire_name = "tire_" + DCCManager::GetContainerDirection(data.model->bone_name);

							FbxNode* locatorObj = nullptr;

							if (!resolver.GetMeshes().empty())
							{
								auto trs = std::find_if(spindle_transforms.begin(), spindle_transforms.end(), [&](auto& d) { return d.first == data.model->bone_name; });

								locatorObj = CreateLocator(trs->second);

								locatorObj->SetName(tire_name.c_str());
							}

							for (auto& mesh : resolver.GetMeshes())
							{
								FbxNode* mesh_obj = nullptr;
								FbxSurfaceLambert* material_obj = nullptr;

								std::string mesh_name{};

								auto material = std::find_if(data.bundle->MaterialInstanceBundles.begin(), data.bundle->MaterialInstanceBundles.end(), [&](auto& mtl) {
									return std::any_cast<int32_t>(mtl.metadata["Id"]) == mesh.material_index;
									});

								if (material != std::end(data.bundle->MaterialInstanceBundles))
								{
									mesh_name += mesh.name;
									mesh_name += "_";
									mesh_name += std::any_cast<std::string>(material->metadata["Name"]);
								}

								if (auto material_it = data.materials.find(mesh.material_index); material_it != std::end(data.materials))
								{
									const auto& [key, material_data] = *material_it;

									material_obj = CreateMaterialfromMemory(std::any_cast<std::string>(material->metadata["Name"]), material_data.instace);
								}

								mesh_obj = CreateMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs, mesh_name, material_obj, static_cast<fmnext::GeometryType>(m_geo));
								SetNodeTransformation(mesh_obj, mesh.matrix);

								locatorObj->AddChild(mesh_obj);
							}

							lodGroupObj->AddChild(locatorObj);
						}

						continue;
					}

					if (data.model->type == "ControlArm") // Suspensions
					{
						std::string suspension_name = "suspension_" + DCCManager::GetContainerDirection(data.model->bone_name);

						FbxNode* locatorObj = CreateLocator(data.model->transform);
						locatorObj->SetName(suspension_name.c_str());

						auto resolver = fmnext::MeshResolver(data.bundle, m_lod, static_cast<fmnext::GeometryType>(m_geo));

						for (auto& mesh : resolver.GetMeshes())
						{
							FbxNode* mesh_obj = nullptr;
							FbxSurfaceLambert* material_obj = nullptr;

							std::string mesh_name{};

							auto material = std::find_if(data.bundle->MaterialInstanceBundles.begin(), data.bundle->MaterialInstanceBundles.end(), [&](auto& mtl) {
								return std::any_cast<int32_t>(mtl.metadata["Id"]) == mesh.material_index;
								});

							if (material != std::end(data.bundle->MaterialInstanceBundles))
							{
								mesh_name += mesh.name;
								mesh_name += "_";
								mesh_name += std::any_cast<std::string>(material->metadata["Name"]);
							}

							if (auto material_it = data.materials.find(mesh.material_index); material_it != std::end(data.materials))
							{
								const auto& [key, material_data] = *material_it;

								material_obj = CreateMaterialfromMemory(std::any_cast<std::string>(material->metadata["Name"]), material_data.instace);
							}

							mesh_obj = CreateMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs, mesh_name, material_obj, static_cast<fmnext::GeometryType>(m_geo));
							SetNodeTransformation(mesh_obj, mesh.matrix);

							locatorObj->AddChild(mesh_obj);
						}

						if (true) //has_db
						{
							suspensions.emplace(data.model->bone_name, locatorObj);
						}

						lodGroupObj->AddChild(locatorObj);

						continue;
					}

					// caliper
					if (data.model->bone_name.find("hub") != std::string::npos && data.model->type == "Brakes")
					{
						std::string caliper_name = "caliper_" + DCCManager::GetContainerDirection(data.model->bone_name);

						FbxNode* locatorObj = CreateLocator(data.model->transform);
						locatorObj->SetName(caliper_name.c_str());

						auto resolver = fmnext::MeshResolver(data.bundle, m_lod, static_cast<fmnext::GeometryType>(m_geo));

						for (auto& mesh : resolver.GetMeshes())
						{
							FbxNode* mesh_obj = nullptr;
							FbxSurfaceLambert* material_obj = nullptr;

							std::string mesh_name{};

							auto material = std::find_if(data.bundle->MaterialInstanceBundles.begin(), data.bundle->MaterialInstanceBundles.end(), [&](auto& mtl) {
								return std::any_cast<int32_t>(mtl.metadata["Id"]) == mesh.material_index;
								});

							if (material != std::end(data.bundle->MaterialInstanceBundles))
							{
								mesh_name += mesh.name;
								mesh_name += "_";
								mesh_name += std::any_cast<std::string>(material->metadata["Name"]);
							}

							if (auto material_it = data.materials.find(mesh.material_index); material_it != std::end(data.materials))
							{
								const auto& [key, material_data] = *material_it;

								material_obj = CreateMaterialfromMemory(std::any_cast<std::string>(material->metadata["Name"]), material_data.instace);
							}

							mesh_obj = CreateMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs, mesh_name, material_obj, static_cast<fmnext::GeometryType>(m_geo));
							SetNodeTransformation(mesh_obj, mesh.matrix);

							locatorObj->AddChild(mesh_obj);
						}

						if (true) //has_db
						{
							calipers.emplace(data.model->bone_name, locatorObj);
							caliper_transforms.emplace(data.model->bone_name, data.model->transform);

							//MGlobal::displayInfo(std::string(path + "\n").c_str());
						}

						lodGroupObj->AddChild(locatorObj);

						continue;
					}

					// rotor
					if (data.model->bone_name.find("spindle") != std::string::npos && data.model->type == "Brakes")
					{
						std::string rotor_name = "rotor_" + DCCManager::GetContainerDirection(data.model->bone_name);

						FbxNode* locatorObj = CreateLocator(data.model->transform);
						locatorObj->SetName(rotor_name.c_str());

						float control_arm_offset = 0.30480003f; // 12 inch(0x3E9C0EC0)

						for (auto& bone : data.bundle->Skeleton)
						{
							if (bone.name == "controlArm")
							{
								control_arm_offset = DirectX::XMVectorGetX(bone.transform.r[3]);
								break;
							}
						}

						//float rotor_center_offset = 0.f;

						if (true) //has_db
						{
							for (auto& bone : data.bundle->Skeleton)
							{
								if (bone.name.find("rotor") != std::string::npos)
								{
									//rotor_center_offset = DirectX::XMVectorGetX(bone.transform.r[3]);
									rotor_center_offsets.emplace(data.model->bone_name, DirectX::XMVectorGetX(bone.transform.r[3]));

									break;
								}
							}
						}

						auto resolver = fmnext::MeshResolver(data.bundle, m_lod, static_cast<fmnext::GeometryType>(m_geo));

						for (auto& mesh : resolver.GetMeshes())
						{
							FbxNode* mesh_obj = nullptr;
							FbxSurfaceLambert* material_obj = nullptr;

							std::string mesh_name{};

							auto material = std::find_if(data.bundle->MaterialInstanceBundles.begin(), data.bundle->MaterialInstanceBundles.end(), [&](auto& mtl) {
								return std::any_cast<int32_t>(mtl.metadata["Id"]) == mesh.material_index;
								});

							if (material != std::end(data.bundle->MaterialInstanceBundles))
							{
								mesh_name += mesh.name;
								mesh_name += "_";
								mesh_name += std::any_cast<std::string>(material->metadata["Name"]);
							}

							if (auto material_it = data.materials.find(mesh.material_index); material_it != std::end(data.materials))
							{
								const auto& [key, material_data] = *material_it;

								material_obj = CreateMaterialfromMemory(std::any_cast<std::string>(material->metadata["Name"]), material_data.instace);
							}

							mesh_obj = CreateMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs, mesh_name, material_obj, static_cast<fmnext::GeometryType>(m_geo));
							SetNodeTransformation(mesh_obj, mesh.matrix);

							locatorObj->AddChild(mesh_obj);
						}

						if (true) //has_db
						{
							rotors.emplace(data.model->bone_name, locatorObj);
							rotor_transforms.emplace(data.model->bone_name, data.model->transform);

							//MGlobal::displayInfo(std::string(path + "\n").c_str());
						}

						lodGroupObj->AddChild(locatorObj);

						continue;
					}

					//DCCManager::objExists(std::filesystem::path(data.model->path).stem().string())
					{
						FbxNode* locatorObj = nullptr;

						auto resolver = fmnext::MeshResolver(data.bundle, m_lod, static_cast<fmnext::GeometryType>(m_geo));

						if (!resolver.GetMeshes().empty())
						{
							locatorObj = CreateLocator(data.model->transform);

							std::string bundle_name = std::filesystem::path(data.model->path).stem().string();

							locatorObj->SetName(bundle_name.c_str());
						}

						for (auto& mesh : resolver.GetMeshes())
						{
							FbxNode* mesh_obj = nullptr;
							FbxSurfaceLambert* material_obj = nullptr;

							std::string mesh_name{};
							std::string material_instance_name{};

							auto material = std::find_if(data.bundle->MaterialInstanceBundles.begin(), data.bundle->MaterialInstanceBundles.end(), [&](auto& mtl) {
								return std::any_cast<int32_t>(mtl.metadata["Id"]) == mesh.material_index;
								});

							if (material != std::end(data.bundle->MaterialInstanceBundles))
							{
								mesh_name += mesh.name;
								mesh_name += "_";
								mesh_name += std::any_cast<std::string>(material->metadata["Name"]);
							}

							if (!material->data.empty())
							{
								auto material_bundle_reader = fmnext::BundleReader(material->data);

								if (material_bundle_reader.Init())
								{
									for (auto& inst : material_bundle_reader.bundle.MaterialInstances)
									{
										//MString message;
										//message.format(MString("Mesh ^1s Material path ^2s"), mesh_name.c_str(), inst.c_str());

										material_instance_name = std::filesystem::path(inst).stem().string();

										//MGlobal::displayInfo(message);
									}
								}
							}

							if (auto material_it = data.materials.find(mesh.material_index); material_it != std::end(data.materials))
							{
								const auto& [key, material_data] = *material_it;

								std::string material_name = std::any_cast<std::string>(material->metadata["Name"]);
								/**/

								bool carpaint_v0 = StringContains(material_name, "carpaint");
								bool carpaint_v1 = StringContains(material_instance_name, "carpaint");
								bool carpaint_v2 = StringContains(material_instance_name, "carpaint_secondary");

								bool glass_clear_v0 = StringContains(material_instance_name, "gls");
								bool glass_clear_v1 = StringContains(material_name, "gls");
								bool gls_clear_custom = StringContains(material_name, "gls_clear_custom");
								bool smooth_glass = StringContains(material_name, "smoothGlass");

								if (m_colors && !m_colors->ManufacturerColors.empty())
								{
									if (carpaint_v0 || carpaint_v1 || carpaint_v2)
									{
										auto carpaint = m_colors->ManufacturerColors[color_override][0].preview_color;
										material_obj = CreateCarpaintfromMemory(std::any_cast<std::string>(material->metadata["Name"]), carpaint);
									}
									else if (glass_clear_v0 || glass_clear_v1 || gls_clear_custom || smooth_glass)
									{
										material_obj = CreateGlassfromMemory(material_name);
									}
									else
									{
										material_obj = CreateMaterialfromMemory(std::any_cast<std::string>(material->metadata["Name"]), material_data.instace);
									}
								}
								else
								{
									material_obj = CreateMaterialfromMemory(std::any_cast<std::string>(material->metadata["Name"]), material_data.instace);
								}
							}

							mesh_obj = CreateMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs, mesh_name, material_obj, static_cast<fmnext::GeometryType>(m_geo));
							SetNodeTransformation(mesh_obj, mesh.matrix);


							locatorObj->AddChild(mesh_obj);
						}

						lodGroupObj->AddChild(locatorObj);
					}
				}

			}

			mRootNode->AddChild(lodGroupObj);
		}


		for (auto& [key, obj] : suspensions)
		{
			if (auto transform = suspension_transforms.find(key); transform != suspension_transforms.end())
			{
				SetNodeTransformation(obj, transform->second);

				//MGlobal::displayInfo("Suspensions found!");
			}
			else {
				//MGlobal::displayWarning("Suspensions not found!");
			}
		}

		for (auto& [key, obj] : calipers)
		{
			std::string spindle_key = "spindle" + DCCManager::GetContainerDirection(key);

			if (auto offset = rotor_center_offsets.find(spindle_key); offset != rotor_center_offsets.end())
			{
				DirectX::XMMATRIX caliper_bone = caliper_transforms[key];
				DirectX::XMMATRIX rotor_bone = rotor_transforms[spindle_key];

				DirectX::XMMATRIX caliper_local_transform{};
				DirectX::XMVECTOR caliper_local_translate{};

				DirectX::XMMATRIX translate_x = DirectX::XMMatrixTranslation(spindle_offsets[spindle_key], 0, 0);

				DirectX::XMMATRIX brake_transform{};
				{
					brake_transform += (translate_x * spindle_transforms[spindle_key]);

					SetNodeTransformation(rotors[spindle_key], brake_transform);
				}

				caliper_local_translate = DirectX::XMVectorSet(offset->second, DirectX::XMVectorGetY(caliper_bone.r[3]) - DirectX::XMVectorGetY(rotor_bone.r[3]), DirectX::XMVectorGetZ(caliper_bone.r[3]) - DirectX::XMVectorGetZ(rotor_bone.r[3]), 1.f);

				auto direction = DCCManager::GetPartDirection(key);

				if (direction.side == fmnext::PartLocation::RIGHT)
				{
					caliper_local_transform += (caliper_bone * DirectX::XMMatrixRotationY(DirectX::XMConvertToRadians(180)));
					caliper_local_translate = DirectX::XMVectorSet(DirectX::XMVectorGetX(caliper_local_translate), DirectX::XMVectorGetY(caliper_local_translate), -DirectX::XMVectorGetZ(caliper_local_translate), DirectX::XMVectorGetW(caliper_local_translate));

					caliper_local_transform.r[3] = caliper_local_translate;
				}
				else
				{
					caliper_local_transform = caliper_bone;

					caliper_local_transform.r[3] = caliper_local_translate;
				}

				DirectX::XMMATRIX caliper_transform{}; // assume that hub_transform == spindle_transform (rotate around Y-axis)
				{
					caliper_transform += (caliper_local_transform * brake_transform);

					SetNodeTransformation(obj, caliper_transform);
				}

				//MGlobal::displayInfo("rotor_center_offset found!");
			}
			else
			{
				//MGlobal::displayWarning("rotor_center_offset not found!");
			}
		}

	}
}

FbxMesh* DCCManager::RemoveIsolatedVertices(FbxMesh* prev_mesh)
{
	// Indices
	std::vector<int32_t> relative_indices(prev_mesh->GetControlPointsCount(), -1);
	std::vector<int32_t> absolute_indices;
	std::vector<bool> vertex_used(prev_mesh->GetControlPointsCount(), false);

	for (int32_t i = 0; i < prev_mesh->GetPolygonCount(); ++i)
	{
		for (int32_t p = 0; p < prev_mesh->GetPolygonSize(i); ++p)
		{
			int32_t index = prev_mesh->GetPolygonVertex(i, p);
			vertex_used[index] = true;
		}
	}

	int32_t new_vertex_id = 0;
	for (int32_t i = 0; i < prev_mesh->GetControlPointsCount(); ++i)
	{
		if (vertex_used[i])
		{
			relative_indices[i] = new_vertex_id;
			absolute_indices.push_back(i);
			new_vertex_id++;
		}
	}

	FbxMesh* result = FbxMesh::Create(mManager, "");
	result->InitControlPoints(static_cast<uint32_t>(absolute_indices.size()));

	{ // Vertices

		FbxVector4* prev_control_points = prev_mesh->GetControlPoints();
		FbxVector4* next_control_points = result->GetControlPoints();

		for (int32_t i = 0; i < absolute_indices.size(); ++i)
		{
			int32_t index = absolute_indices[i];
			next_control_points[i] = prev_control_points[index];
		}
	}

	{ // Faces

		for (int32_t i = 0; i < prev_mesh->GetPolygonCount(); ++i)
		{
			result->BeginPolygon(-1, -1, -1, false);

			for (int32_t j = 0; j < prev_mesh->GetPolygonSize(i); ++j)
			{
				int32_t prev_index = prev_mesh->GetPolygonVertex(i, j);
				int32_t next_index = relative_indices[prev_index];
				result->AddPolygon(next_index);
			}

			result->EndPolygon();
		}
	}

	{ // Normals

		FbxLayerElementNormal* prev_mesh_normals = prev_mesh->GetLayer(0)->GetNormals();
		FbxLayerElementNormal* next_mesh_normals = result->CreateElementNormal();

		next_mesh_normals->SetMappingMode(prev_mesh_normals->GetMappingMode());
		next_mesh_normals->SetReferenceMode(prev_mesh_normals->GetReferenceMode());

		for (int32_t i = 0; i < absolute_indices.size(); ++i)
		{
			next_mesh_normals->GetDirectArray().Add(prev_mesh_normals->GetDirectArray().GetAt(absolute_indices[i]));
		}
	}

	for (uint32_t id = 0; id < static_cast<uint32_t>(prev_mesh->GetLayerCount()); ++id)
	{
		// UVs
		FbxLayerElementUV* prev_mesh_uv = prev_mesh->GetLayer(id)->GetUVs();
		FbxGeometryElementUV* new_mesh_uv = result->CreateElementUV(prev_mesh_uv->GetName());

		new_mesh_uv->SetMappingMode(prev_mesh_uv->GetMappingMode());
		new_mesh_uv->SetReferenceMode(prev_mesh_uv->GetReferenceMode());

		std::vector<bool> uv_used(prev_mesh_uv->GetDirectArray().GetCount(), false);
		for (int32_t i = 0; i < prev_mesh_uv->GetIndexArray().GetCount(); ++i)
		{
			int32_t index = prev_mesh_uv->GetIndexArray().GetAt(i);
			if (index >= 0 && index < prev_mesh_uv->GetDirectArray().GetCount())
			{
				uv_used[index] = true;
			}
		}

		std::vector<int32_t> rel_uv_indices(prev_mesh_uv->GetDirectArray().GetCount(), -1);

		int32_t new_uv_id = 0;
		for (int32_t i = 0; i < prev_mesh_uv->GetDirectArray().GetCount(); ++i)
		{
			if (uv_used[i])
			{
				new_mesh_uv->GetDirectArray().Add(prev_mesh_uv->GetDirectArray().GetAt(i));

				rel_uv_indices[i] = new_uv_id;
				new_uv_id++;
			}
		}

		for (int32_t i = 0; i < prev_mesh_uv->GetIndexArray().GetCount(); ++i)
		{
			int32_t index = prev_mesh_uv->GetIndexArray().GetAt(i);
			int32_t mapped_index = (index >= 0 && index < prev_mesh_uv->GetDirectArray().GetCount()) ? rel_uv_indices[index] : -1;

			new_mesh_uv->GetIndexArray().Add(mapped_index);
		}
	}

	return result;
}

FbxNode* DCCManager::CreateMesh(const std::vector<DirectX::XMFLOAT3>& vertices, const std::vector<uint32_t>& indices, const std::vector<DirectX::XMFLOAT3>& normals, const std::vector<std::vector<DirectX::XMFLOAT2>>& uvs, const std::string& Name, FbxSurfaceMaterial* material, bool useQuads)
{
	FbxMesh* lMesh = FbxMesh::Create(mManager, "");

	uint32_t geometry = (useQuads) ? 4 : 3;
	uint32_t numVertices = static_cast<int>(vertices.size()); // verts
	uint32_t numIndices = static_cast<int>(indices.size());
	uint32_t numPolygons = static_cast<int>(numIndices / geometry); // faces

	// Create control points.
	lMesh->InitControlPoints(numVertices);
	FbxVector4* lControlPoints = lMesh->GetControlPoints();

	for (uint32_t i = 0; i < numVertices; ++i)
	{
		lControlPoints[i] = FbxVector4(vertices[i].x, vertices[i].z, vertices[i].y);
	}

	FbxGeometryElementNormal* lElementNormal = lMesh->CreateElementNormal();

	lElementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);
	lElementNormal->SetReferenceMode(FbxGeometryElement::eDirect);

	for (uint32_t i = 0; i < numVertices; ++i)
	{
		lElementNormal->GetDirectArray().Add(FbxVector4(normals[i].x, normals[i].z, normals[i].y));
	}

	for (uint32_t id = 0; id < static_cast<uint32_t>(uvs.size()) && !uvs[id].empty(); ++id)
	{
		// UVs Set {ID}
		std::string uvSet = "UVChannel_";
		uvSet += std::to_string(id + 1).c_str();

		FbxGeometryElementUV* meshUV = lMesh->CreateElementUV(uvSet.c_str());
		meshUV->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
		meshUV->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

		for (uint32_t i = 0; i < numIndices; i += geometry)
		{
			uint32_t v0 = indices[i + 0];
			uint32_t v1 = (geometry == 4) ? indices[i + 2] : indices[i + 1];
			uint32_t v2 = (geometry == 4) ? indices[i + 1] : indices[i + 2];
			uint32_t v3 = (geometry == 4) ? indices[i + 3] : 0xffffffff;

			meshUV->GetIndexArray().Add(v0);
			meshUV->GetIndexArray().Add(v2);
			meshUV->GetIndexArray().Add(v1);

			if (v3 != 0xffffffff) {
				meshUV->GetIndexArray().Add(v3);
			}
		}

		for (uint32_t i = 0; i < static_cast<uint32_t>(uvs[id].size()); ++i)
		{
			meshUV->GetDirectArray().Add(FbxVector2(uvs[id][i].x, 1 - uvs[id][i].y));
		}
	}


	for (uint32_t i = 0; i < numIndices; i += geometry) //numIndices
	{
		lMesh->BeginPolygon(-1, -1, false);
		{
			uint32_t v0 = indices[i + 0];
			uint32_t v1 = (geometry == 4) ? indices[i + 1] : indices[i + 2];
			uint32_t v2 = (geometry == 4) ? indices[i + 2] : indices[i + 1];
			uint32_t v3 = (geometry == 4) ? indices[i + 3] : 0xffffffff;

			lMesh->AddPolygon(v0);
			lMesh->AddPolygon(v1);
			lMesh->AddPolygon(v2);

			if (v3 != 0xffffffff) {
				lMesh->AddPolygon(v3);
			}
		}
		lMesh->EndPolygon();
	}

	lMesh->BuildMeshEdgeArray();

	// remove overlapping vertices
	lMesh->RemoveBadPolygons();

	// create a FbxNode
	FbxNode* lNode = FbxNode::Create(mManager, Name.c_str());

	if (m_opt == 1)
	{
		// set the node attribute
		lNode->SetNodeAttribute(RemoveIsolatedVertices(lMesh));
		lMesh->Destroy();
	}
	else {
		// set the node attribute
		lNode->SetNodeAttribute(lMesh);
	}

	// set the shading mode to view texture
	lNode->SetShadingMode(FbxNode::eTextureShading);

	// add material
	lNode->AddMaterial(material);

	// return the FbxNode
	return lNode;
}


granny_file_info* GrannyBindingCallback(gstate_character_info* BindingInfo, char const* SourceFilename, void* UserData)
{
	//printf("SourceFilename: %s \n", SourceFilename);

	auto result = std::find(references.begin(), references.end(), std::string(SourceFilename));
	if (result == references.end())
	{
		references.push_back(std::string(SourceFilename));
	}

	return nullptr;
}


int DCCManager::InitStateMachine(const std::vector<char>& state)
{
	if (!GrannyVersionsMatch)
	{
		printf("Warning: the Granny DLL currently loaded "
			"doesn't match the .h file used during compilation\n");
		return EXIT_FAILURE;
	}

	granny_file* CharacterFile = 0;
	gstate_character_info* CharacterInfo = 0;
	granny_file_reader* StateFile = GrannyCreateMemoryFileReader(static_cast<granny_int32x>(state.size()), state.data());

	if (GStateReadCharacterInfoFromReader(StateFile, CharacterFile, CharacterInfo) == false)
	{
		// handle error and bail
		return EXIT_FAILURE;
	}

	for (granny_int32x i = 0; i < GStateGetNumAnimationSets(CharacterInfo); ++i)
	{
		std::string AnimationSetName = GStateGetAnimationSetName(CharacterInfo, i);

		if (GStateBindCharacterFileReferences(CharacterInfo, GrannyBindingCallback, 0) == false)
		{
			// handle error and bail
			//return EXIT_FAILURE;
		}

		printf("\tContextUID_state_machine: %s \n", AnimationSetName.c_str());
	}

	GrannyFreeFile(CharacterFile);
	GrannyCloseFileReader(StateFile);

	CharacterInfo = 0;
	CharacterFile = 0;

	return EXIT_SUCCESS;
}

rapidjson::Value DCCManager::StringToValue(const std::string& value, rapidjson::Document::AllocatorType& allocator)
{
	rapidjson::Value result(rapidjson::kStringType);
	result.SetString(value.c_str(), static_cast<rapidjson::SizeType>(value.size()), allocator);

	return result;
}

void DCCManager::ExportManufacturerColors()
{
	rapidjson::Document json_document{};
	json_document.SetObject();

	rapidjson::Value document_entries(rapidjson::kArrayType);

	rapidjson::Value metadata_object(rapidjson::kObjectType);
	metadata_object.AddMember("version", 1, json_document.GetAllocator());
	metadata_object.AddMember("type", "ManufacturerColors", json_document.GetAllocator());
	metadata_object.AddMember("generator", "ForzaTech CLI Toolkit", json_document.GetAllocator());

	json_document.AddMember("metadata", metadata_object, json_document.GetAllocator());

	if (m_colors != nullptr) {

		for (auto it = m_colors->ManufacturerColors.begin(); it != m_colors->ManufacturerColors.end(); ++it)
		{
			size_t index = std::distance(m_colors->ManufacturerColors.begin(), it);

			rapidjson::Value json_object(rapidjson::kObjectType);
			json_object.AddMember("Color_Set", index, json_document.GetAllocator());

			rapidjson::Value array(rapidjson::kArrayType);

			for (auto colors = it->begin(); colors != it->end(); ++colors)
			{
				size_t idx = std::distance(it->begin(), colors);

				rapidjson::Value color_object(rapidjson::kObjectType);

				color_object.AddMember("Index_Mask", colors->material_index_mask.value(), json_document.GetAllocator());
				color_object.AddMember("Path", StringToValue(colors->path, json_document.GetAllocator()), json_document.GetAllocator());

				rapidjson::Value preview_color(rapidjson::kArrayType);
				preview_color.PushBack(colors->preview_color.x, json_document.GetAllocator());
				preview_color.PushBack(colors->preview_color.y, json_document.GetAllocator());
				preview_color.PushBack(colors->preview_color.z, json_document.GetAllocator());

				color_object.AddMember("Preview_Color", preview_color, json_document.GetAllocator());

				{
					std::string path = m_game->Remove(colors->path).string();
					std::replace(path.begin(), path.end(), '\\', '/');

					std::vector<char> blob = FindAssetInContainer(path, 0);

					if (!blob.empty())
					{
						color_object.AddMember("Shader_Parameters", GetShaderParametersArray(GetBundleData(blob), json_document.GetAllocator()), json_document.GetAllocator());
						blob.clear();
					}
				}
				array.PushBack(color_object, json_document.GetAllocator());
			}

			json_object.AddMember("Data", array, json_document.GetAllocator());
			document_entries.PushBack(json_object, json_document.GetAllocator());
		}
		json_document.AddMember("Colors", document_entries, json_document.GetAllocator());
	}

	std::filesystem::path fpath(mOutputPath);
	fpath /= "ManufacturerColors.json";
	fpath.make_preferred();

	if (!std::filesystem::exists(fpath))
	{
		std::ofstream ostream(fpath);
		rapidjson::OStreamWrapper osw(ostream);

		rapidjson::PrettyWriter<rapidjson::OStreamWrapper, rapidjson::UTF8<>> writer(osw);
		writer.SetIndent(' ', 4);
		if (json_document.Accept(writer))
		{
			ostream.close();
			writer.Flush();
		}
	}
}

std::string DCCManager::GetHexHash(int value)
{
	std::stringstream result;
	result << std::uppercase << std::hex << value;

	return std::string(result.str());
}

rapidjson::Value DCCManager::GetShaderParametersArray(std::shared_ptr<fmnext::BundleReader::BundleData> bundle, rapidjson::Document::AllocatorType& allocator)
{
	rapidjson::Value array(rapidjson::kArrayType);

	for (auto it = bundle->ShaderParameters.begin(); it != bundle->ShaderParameters.end(); ++it) {
		uint32_t itx = static_cast<uint32_t>(std::distance(bundle->ShaderParameters.begin(), it));

		switch (it->type) {
		case fmnext::ShaderParameter_Vector: {
			auto result = std::any_cast<DirectX::XMFLOAT4>(it->value);

			rapidjson::Value object(rapidjson::kObjectType);

			rapidjson::Value jsonArray(rapidjson::kArrayType);
			jsonArray.PushBack(result.x, allocator);
			jsonArray.PushBack(result.y, allocator);
			jsonArray.PushBack(result.z, allocator);
			jsonArray.PushBack(result.w, allocator);

			rapidjson::Value Name(rapidjson::kNullType);
			if (strcmp(fmnext::NameHashToString(it->hash), "null") != 0)
			{
				Name.SetString(rapidjson::StringRef(fmnext::NameHashToString(it->hash)), allocator);
			}

			object.AddMember("Id", itx, allocator);
			object.AddMember("Hash", StringToValue(GetHexHash(it->hash), allocator), allocator);
			object.AddMember("Name", Name, allocator);
			object.AddMember("GUID", StringToValue(GetStringGUIDWithoutBraces(GetStringGUID(it->guid)), allocator), allocator);
			object.AddMember("Type", "Vector", allocator);
			object.AddMember("Data", jsonArray, allocator);

			array.PushBack(object, allocator);

			break;
		}
		case fmnext::ShaderParameter_Color: {
			auto result = std::any_cast<DirectX::XMFLOAT4>(it->value);

			rapidjson::Value object(rapidjson::kObjectType);

			rapidjson::Value jsonArray(rapidjson::kArrayType);
			jsonArray.PushBack(result.x, allocator);
			jsonArray.PushBack(result.y, allocator);
			jsonArray.PushBack(result.z, allocator);
			jsonArray.PushBack(result.w, allocator);

			rapidjson::Value Name(rapidjson::kNullType);
			if (strcmp(fmnext::NameHashToString(it->hash), "null") != 0)
			{
				Name.SetString(rapidjson::StringRef(fmnext::NameHashToString(it->hash)), allocator);
			}
			
			object.AddMember("Id", itx, allocator);
			object.AddMember("Hash", StringToValue(GetHexHash(it->hash), allocator), allocator);
			object.AddMember("Name", Name, allocator);
			object.AddMember("GUID", StringToValue(GetStringGUIDWithoutBraces(GetStringGUID(it->guid)), allocator), allocator);
			object.AddMember("Type", "Color", allocator);
			object.AddMember("Data", jsonArray, allocator);

			array.PushBack(object, allocator);

			break;
		}
		case fmnext::ShaderParameter_Float: {
			auto result = std::any_cast<float>(it->value);

			rapidjson::Value object(rapidjson::kObjectType);

			rapidjson::Value Name(rapidjson::kNullType);
			if (strcmp(fmnext::NameHashToString(it->hash), "null") != 0)
			{
				Name.SetString(rapidjson::StringRef(fmnext::NameHashToString(it->hash)), allocator);
			}

			object.AddMember("Id", itx, allocator);
			object.AddMember("Hash", StringToValue(GetHexHash(it->hash), allocator), allocator);
			object.AddMember("Name", Name, allocator);
			object.AddMember("GUID", StringToValue(GetStringGUIDWithoutBraces(GetStringGUID(it->guid)), allocator), allocator);
			object.AddMember("Type", "Float", allocator);
			object.AddMember("Data", result, allocator);

			array.PushBack(object, allocator);

			break;
		}
		case fmnext::ShaderParameter_Bool: {
			auto result = std::any_cast<bool>(it->value);

			rapidjson::Value object(rapidjson::kObjectType);

			rapidjson::Value Name(rapidjson::kNullType);
			if (strcmp(fmnext::NameHashToString(it->hash), "null") != 0)
			{
				Name.SetString(rapidjson::StringRef(fmnext::NameHashToString(it->hash)), allocator);
			}

			object.AddMember("Id", itx, allocator);
			object.AddMember("Hash", StringToValue(GetHexHash(it->hash), allocator), allocator);
			object.AddMember("Name", Name, allocator);
			object.AddMember("GUID", StringToValue(GetStringGUIDWithoutBraces(GetStringGUID(it->guid)), allocator), allocator);
			object.AddMember("Type", "Bool", allocator);
			object.AddMember("Data", result, allocator);

			array.PushBack(object, allocator);

			break;
		}
		case fmnext::ShaderParameter_Int:
		{
			auto result = std::any_cast<int32_t>(it->value);

			rapidjson::Value object(rapidjson::kObjectType);

			rapidjson::Value Name(rapidjson::kNullType);
			if (strcmp(fmnext::NameHashToString(it->hash), "null") != 0)
			{
				Name.SetString(rapidjson::StringRef(fmnext::NameHashToString(it->hash)), allocator);
			}

			object.AddMember("Id", itx, allocator);
			object.AddMember("Hash", StringToValue(GetHexHash(it->hash), allocator), allocator);
			object.AddMember("Name", Name, allocator);
			object.AddMember("GUID", StringToValue(GetStringGUIDWithoutBraces(GetStringGUID(it->guid)), allocator), allocator);
			object.AddMember("Type", "Int", allocator);
			object.AddMember("Data", result, allocator);

			array.PushBack(object, allocator);

			break;
		}
		case fmnext::ShaderParameter_Swizzle: {
			auto result = std::any_cast<std::array<uint8_t, 16>>(it->value);

			rapidjson::Value object(rapidjson::kObjectType);

			rapidjson::Value Name(rapidjson::kNullType);
			if (strcmp(fmnext::NameHashToString(it->hash), "null") != 0)
			{
				Name.SetString(rapidjson::StringRef(fmnext::NameHashToString(it->hash)), allocator);
			}

			object.AddMember("Id", itx, allocator);
			object.AddMember("Hash", StringToValue(GetHexHash(it->hash), allocator), allocator);
			object.AddMember("Name", Name, allocator);
			object.AddMember("GUID", StringToValue(GetStringGUIDWithoutBraces(GetStringGUID(it->guid)), allocator), allocator);
			object.AddMember("Type", "Swizzle", allocator);
			object.AddMember("Data", "No suitable data parser defined.", allocator);

			array.PushBack(object, allocator);

			break;
		}
		case fmnext::ShaderParameter_Texture2D: {
			auto result = std::any_cast<std::string>(it->value);

			rapidjson::Value object(rapidjson::kObjectType);

			rapidjson::Value Name(rapidjson::kNullType);
			if (strcmp(fmnext::NameHashToString(it->hash), "null") != 0)
			{
				Name.SetString(rapidjson::StringRef(fmnext::NameHashToString(it->hash)), allocator);
			}

			object.AddMember("Id", itx, allocator);
			object.AddMember("Hash", StringToValue(GetHexHash(it->hash), allocator), allocator);
			object.AddMember("Name", Name, allocator);
			object.AddMember("GUID", StringToValue(GetStringGUIDWithoutBraces(GetStringGUID(it->guid)), allocator), allocator);
			object.AddMember("Type", "Texture2D", allocator);
			object.AddMember("Data", StringToValue(result, allocator), allocator);

			array.PushBack(object, allocator);

			break;
		}
		case fmnext::ShaderParameter_Vector2:
		{
			auto result = std::any_cast<DirectX::XMFLOAT2>(it->value);

			rapidjson::Value object(rapidjson::kObjectType);

			rapidjson::Value jsonArray(rapidjson::kArrayType);
			jsonArray.PushBack(result.x, allocator);
			jsonArray.PushBack(result.y, allocator);

			rapidjson::Value Name(rapidjson::kNullType);
			if (strcmp(fmnext::NameHashToString(it->hash), "null") != 0)
			{
				Name.SetString(rapidjson::StringRef(fmnext::NameHashToString(it->hash)), allocator);
			}

			object.AddMember("Id", itx, allocator);
			object.AddMember("Hash", StringToValue(GetHexHash(it->hash), allocator), allocator);
			object.AddMember("Name", Name, allocator);
			object.AddMember("GUID", StringToValue(GetStringGUIDWithoutBraces(GetStringGUID(it->guid)), allocator), allocator);
			object.AddMember("Type", "Vector2", allocator);
			object.AddMember("Data", jsonArray, allocator);

			array.PushBack(object, allocator);

			break;
		}
		case fmnext::ShaderParameter_Sampler:
		case fmnext::ShaderParameter_ColorGradient:
		case fmnext::ShaderParameter_FunctionRange:
			break;
		}
	};

	return array;
}


void DCCManager::ExportThumbnail(std::unique_ptr<fmnext::BundleReader::BundleData> ptr, std::string pFile)
{
	std::filesystem::path lOutputPath(mOutputPath);
	lOutputPath /= std::filesystem::path(pFile).filename();
	lOutputPath.make_preferred();

	if (ptr && !std::filesystem::exists(lOutputPath))
	{
		auto texture_resolver = fmnext::TextureResolver(*ptr);
		texture_resolver.SaveToPNGFile(lOutputPath.string());
	}
};