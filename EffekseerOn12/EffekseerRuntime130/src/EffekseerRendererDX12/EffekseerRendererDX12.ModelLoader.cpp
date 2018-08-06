

#include "EffekseerRendererDX12.Renderer.h"
#include "EffekseerRendererDX12.ModelLoader.h"
#include<memory>

namespace EffekseerRendererDX12
{
	ModelLoader::ModelLoader(ID3D12Device* device, ::Effekseer::FileInterface* fileInterface)
		: device(device)
		, m_fileInterface(fileInterface)
	{
		ES_SAFE_ADDREF(device);

		if (m_fileInterface == nullptr)
		{
			m_fileInterface = &m_defaultFileInterface;
		}
	}

	ModelLoader::~ModelLoader()
	{
		ES_SAFE_RELEASE(device);
	}

	void* ModelLoader::Load(const EFK_CHAR* path)
	{
		std::shared_ptr<::Effekseer::FileReader>
			reader(m_fileInterface->OpenRead(path));
		if (reader)
		{
			return nullptr;
		}
		else
		{
			size_t size_model = reader->GetLength();
			uint8_t* data_model = new uint8_t[size_model];
			reader->Read(data_model, size_model);

			Model* model = new Model(data_model, size_model);

			model->ModelCount = Effekseer::Min(Effekseer::Max(model->GetModelCount(), 1), 40);

			model->VertexCount = model->GetVertexCount();

			if (model->VertexCount == 0)
			{
				return nullptr;
			}

			{
				std::vector<Effekseer::Model::VertexWithIndex> vs;

				for (int32_t m = 0; m < model->ModelCount; ++m)
				{
					for (int32_t i = 0; i < model->GetVertexCount(); ++i)
					{
						Effekseer::Model::VertexWithIndex v;
						v.Position = model->GetVertexes()[i].Position;
						v.Normal = model->GetVertexes()[i].Normal;
						v.Binormal = model->GetVertexes()[i].Binormal;
						v.Tangent = model->GetVertexes()[i].Tangent;
						v.UV = model->GetVertexes()[i].UV;
						v.VColor = model->GetVertexes()[i].VColor;
						v.Index[0] = m;

						vs.push_back(v);
					}
				}

				/*
				ID3D11Buffer* vb = NULL;

				D3D11_BUFFER_DESC hBufferDesc;
				hBufferDesc.ByteWidth = sizeof(Effekseer::Model::VertexWithIndex) * model->GetVertexCount() * model->ModelCount;
				hBufferDesc.Usage = D3D11_USAGE_DEFAULT;
				hBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				hBufferDesc.CPUAccessFlags = 0;
				hBufferDesc.MiscFlags = 0;
				hBufferDesc.StructureByteStride = sizeof(float);

				D3D11_SUBRESOURCE_DATA hSubResourceData;
				hSubResourceData.pSysMem = &(vs[0]);
				hSubResourceData.SysMemPitch = 0;
				hSubResourceData.SysMemSlicePitch = 0;

				if (FAILED(device->CreateBuffer(&hBufferDesc, &hSubResourceData, &vb)))
				{
					return NULL;
				}

				model->VertexBuffer = vb;
				*/

				ComPtr<ID3D12Resource> vb = nullptr;
				D3D12_RESOURCE_DESC hBufferDesc;
				hBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				hBufferDesc.Alignment = 0;
				hBufferDesc.Width = sizeof(Effekseer::Model::VertexWithIndex) *  model->GetVertexCount() * model->ModelCount;
				hBufferDesc.Height = 1;
				hBufferDesc.DepthOrArraySize = 1;
				hBufferDesc.MipLevels = 1;
				hBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
				hBufferDesc.SampleDesc.Count = 1;
				hBufferDesc.SampleDesc.Quality = 0;
				hBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				hBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			}


			model->FaceCount = model->GetFaceCount();

			/* 0.50より追加(0.50以前から移行する時は追記する必要あり) */
			model->IndexCount = model->FaceCount * 3;

			{
				std::vector<Effekseer::Model::Face> fs;
				for (int32_t m = 0; m < model->ModelCount; m++)
				{
					for (int32_t i = 0; i < model->FaceCount; i++)
					{
						Effekseer::Model::Face f;
						f.Indexes[0] = model->GetFaces()[i].Indexes[0] + model->GetVertexCount() * m;
						f.Indexes[1] = model->GetFaces()[i].Indexes[1] + model->GetVertexCount() * m;
						f.Indexes[2] = model->GetFaces()[i].Indexes[2] + model->GetVertexCount() * m;
						fs.push_back(f);
					}
				}

				// todo
				/*
				ID3D11Buffer* ib = NULL;
				D3D11_BUFFER_DESC hBufferDesc;
				hBufferDesc.Usage = D3D11_USAGE_DEFAULT;
				hBufferDesc.ByteWidth = sizeof(int32_t) * 3 * model->FaceCount * model->ModelCount;
				hBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				hBufferDesc.CPUAccessFlags = 0;
				hBufferDesc.MiscFlags = 0;
				hBufferDesc.StructureByteStride = sizeof(int32_t);

				D3D11_SUBRESOURCE_DATA hSubResourceData;
				hSubResourceData.pSysMem = &(fs[0]);
				hSubResourceData.SysMemPitch = 0;
				hSubResourceData.SysMemSlicePitch = 0;

				if (FAILED(device->CreateBuffer(&hBufferDesc, &hSubResourceData, &ib)))
				{
					return NULL;
				}

				model->IndexBuffer = ib;
				*/
			}

			delete[] data_model;

			return (void*)model;

		}
		return nullptr;

	}

	void ModelLoader::Unload(void* data)
	{
		if (data != nullptr)
		{
			Model* model = (Model*)data;
			delete model;
		}
	}
}