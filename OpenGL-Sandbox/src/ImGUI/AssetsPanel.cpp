#include "AssetsPanel.h"
#include "glpch.h"
#include <imgui.h>
#include <iostream>



AssetsPanel::AssetsPanel()
{
	m_CurrentDirectory = s_AssetPath;
}

void AssetsPanel::setIcons(GLuint folderTexture, GLuint modelTexture, GLuint imageTexture)
{
	this->folderTexture = folderTexture;
	this->modelTexture = modelTexture;
	this->imageTexture = imageTexture;
}




void AssetsPanel::OnImGuiRender()
{
	ImGui::Begin("Assets", nullptr);

	ImGui::BeginChild("FileRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

	if (m_CurrentDirectory != std::filesystem::path(s_AssetPath))
	{
		if (ImGui::Button("<-"))
		{
			m_CurrentDirectory = m_CurrentDirectory.parent_path();
		}
	}

	static float padding = 0.0f;
	static float thumbnailSize = 64.0f;
	float cellSize = thumbnailSize + padding;

	float panelWidth = ImGui::GetContentRegionAvail().x;
	int columnCount = thumbnailSize < 40 ? 1 : (int)(panelWidth / cellSize);

	if (columnCount < 1)
		columnCount = 1;

	ImGui::Columns(columnCount, 0, false);



	for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
	{
		const auto& path = directoryEntry.path();
		auto relativePath = std::filesystem::relative(path, s_AssetPath);
		std::string filenameString = path.filename().string();

		ImGui::PushID(filenameString.c_str());

		GLuint iconTexture = 0; // Reemplazar con la ID de tu textura real


		if (directoryEntry.is_directory())
		{
			iconTexture = folderTexture; // Asumiendo que folderTexture es la ID de tu textura para carpetas
		}
		else if (path.extension() == ".fbx" || path.extension() == ".obj" || path.extension() == ".gltf")
		{
			iconTexture = modelTexture; // Asumiendo que modelTexture es la ID de tu textura para modelos
		}
		else if (path.extension() == ".png" || path.extension() == ".jpg")
		{
			iconTexture = imageTexture; // Asumiendo que imageTexture es la ID de tu textura para imágenes

			// Carga la textura desde el archivo
			//GLuint texture = GLCore::Loaders::LoadIconTexture(filenameString.c_str());
			//ImGui::ImageButton((ImTextureID)(intptr_t)texture, ImVec2(thumbnailSize, thumbnailSize));
			//iconTexture = texture; // Asumiendo que imageTexture es la ID de tu textura para imágenes
		}

		if (iconTexture != 0)
		{
			ImGui::ImageButton((ImTextureID)(intptr_t)iconTexture, ImVec2(thumbnailSize - 15, thumbnailSize - 15));
		}
		else
		{
			ImGui::Button(filenameString.c_str(), { thumbnailSize, thumbnailSize });
		}


		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
		{
			if (directoryEntry.is_directory())
			{
				m_CurrentDirectory /= path.filename();
			}
			else
			{
				std::filesystem::path fullPath = directoryEntry.path();
				std::string filePath = fullPath.parent_path().string();
				std::string fileName = fullPath.filename().string();

				// Agrega una barra al final de la ruta si no está ya presente
				if (filePath.back() != '\\')
				{
					filePath += '\\';
				}

				std::cout << "Selected file: " << fullPath << std::endl;
				std::cout << "File path: " << filePath << std::endl;
				std::cout << "File name: " << fileName << std::endl;

				UsarDelegado(filePath, fileName);
			}
		};

		// Si este elemento se está arrastrando...
		if (ImGui::IsItemHovered() && ImGui::IsMouseDragging())
		{
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
			{
				const std::string payload_n = relativePath.string();
				ImGui::SetDragDropPayload("ASSET_DRAG", payload_n.c_str(), payload_n.length() + 1, ImGuiCond_Once);
				
				std::cout << "File name: " << payload_n << std::endl;
				ImGui::EndDragDropSource();
			}
		}

		ImGui::Text(filenameString.c_str());
		ImGui::NextColumn();

		ImGui::PopID();
	}

	ImGui::Columns(1);

	ImGui::EndChild();
	ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
	ImGui::End();
}
