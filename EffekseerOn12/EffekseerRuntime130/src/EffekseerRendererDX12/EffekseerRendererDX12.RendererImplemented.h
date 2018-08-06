
#ifndef	__EFFEKSEERRENDERER_DX12_RENDERER_IMPLEMENTED_H__
#define	__EFFEKSEERRENDERER_DX12_RENDERER_IMPLEMENTED_H__
// Include

#include "EffekseerRendererDx12.Base.h"
#include "EffekseerRendererDx12.Renderer.h"
#include "EffekseerRendererCommon\EffekseerRenderer.RenderStateBase.h"
#include "EffekseerRendererCommon\EffekseerRenderer.StandardRenderer.h"

#ifdef _MSC_VER
#include<xmmintrin.h>
#endif

namespace EffekseerRendererDX12
{
	struct Vertex
	{
		::Effekseer::Vector3D Pos;
		uint8_t		Col[4];
		float		UV[2];

		void SetColor(const ::Effekseer::Color& color)
		{
			Col[0] = color.R;
			Col[1] = color.G;
			Col[2] = color.B;
			Col[3] = color.A;
		}
	};

	struct VertexDistortion
	{
		::Effekseer::Vector3D	Pos;
		uint8_t		Col[4];
		float		UV[2];
		::Effekseer::Vector3D	Tangent;
		::Effekseer::Vector3D	Binormal;

		void SetColor(const ::Effekseer::Color& color)
		{
			Col[0] = color.R;
			Col[1] = color.G;
			Col[2] = color.B;
			Col[3] = color.A;
		}
	};

	inline void TransformVertexes(Vertex* vertexes, int32_t count, const ::Effekseer::Matrix43& mat)
	{
#if 1
		__m128 r0 = _mm_loadu_ps(mat.Value[0]);
		__m128 r1 = _mm_loadu_ps(mat.Value[1]);
		__m128 r2 = _mm_loadu_ps(mat.Value[2]);
		__m128 r3 = _mm_loadu_ps(mat.Value[3]);

		float tmp_out[4];
		::Effekseer::Vector3D* inout_prev;

		// �P���[�v��
		{
			::Effekseer::Vector3D* inout_cur = &vertexes[0].Pos;
			__m128 v = _mm_loadu_ps((const float*)inout_cur);

			__m128 x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 a0 = _mm_mul_ps(r0, x);
			__m128 y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 a1 = _mm_mul_ps(r1, y);
			__m128 z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 a2 = _mm_mul_ps(r2, z);

			__m128 a01 = _mm_add_ps(a0, a1);
			__m128 a23 = _mm_add_ps(a2, r3);
			__m128 a = _mm_add_ps(a01, a23);

			// ����̌��ʂ��X�g�A���Ă���
			_mm_storeu_ps(tmp_out, a);
			inout_prev = inout_cur;
		}

		for (int i = 1; i < count; i++)
		{
			::Effekseer::Vector3D* inout_cur = &vertexes[i].Pos;
			__m128 v = _mm_loadu_ps((const float*)inout_cur);

			__m128 x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 a0 = _mm_mul_ps(r0, x);
			__m128 y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 a1 = _mm_mul_ps(r1, y);
			__m128 z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 a2 = _mm_mul_ps(r2, z);

			__m128 a01 = _mm_add_ps(a0, a1);
			__m128 a23 = _mm_add_ps(a2, r3);
			__m128 a = _mm_add_ps(a01, a23);

			// ���O�̃��[�v�̌��ʂ��������݂܂�
			inout_prev->X = tmp_out[0];
			inout_prev->Y = tmp_out[1];
			inout_prev->Z = tmp_out[2];

			// ����̌��ʂ��X�g�A���Ă���
			_mm_storeu_ps(tmp_out, a);
			inout_prev = inout_cur;
		}

		// �Ō�̃��[�v�̌��ʂ���������
		{
			inout_prev->X = tmp_out[0];
			inout_prev->Y = tmp_out[1];
			inout_prev->Z = tmp_out[2];
		}

#else
		for (int i = 0; i < count; i++)
		{
			::Effekseer::Vector3D::Transform(
				vertexes[i].Pos,
				vertexes[i].Pos,
				mat);
		}
#endif
	}

	inline void TransformVertexes(VertexDistortion* vertexes, int32_t count, const ::Effekseer::Matrix43& mat)
	{
#if 1
		__m128 r0 = _mm_loadu_ps(mat.Value[0]);
		__m128 r1 = _mm_loadu_ps(mat.Value[1]);
		__m128 r2 = _mm_loadu_ps(mat.Value[2]);
		__m128 r3 = _mm_loadu_ps(mat.Value[3]);

		float tmp_out[4];
		::Effekseer::Vector3D* inout_prev;

		// �P���[�v��
		{
			::Effekseer::Vector3D* inout_cur = &vertexes[0].Pos;
			__m128 v = _mm_loadu_ps((const float*)inout_cur);

			__m128 x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 a0 = _mm_mul_ps(r0, x);
			__m128 y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 a1 = _mm_mul_ps(r1, y);
			__m128 z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 a2 = _mm_mul_ps(r2, z);

			__m128 a01 = _mm_add_ps(a0, a1);
			__m128 a23 = _mm_add_ps(a2, r3);
			__m128 a = _mm_add_ps(a01, a23);

			// ����̌��ʂ��X�g�A���Ă���
			_mm_storeu_ps(tmp_out, a);
			inout_prev = inout_cur;
		}

		for (int i = 1; i < count; i++)
		{
			::Effekseer::Vector3D* inout_cur = &vertexes[i].Pos;
			__m128 v = _mm_loadu_ps((const float*)inout_cur);

			__m128 x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 a0 = _mm_mul_ps(r0, x);
			__m128 y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 a1 = _mm_mul_ps(r1, y);
			__m128 z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 a2 = _mm_mul_ps(r2, z);

			__m128 a01 = _mm_add_ps(a0, a1);
			__m128 a23 = _mm_add_ps(a2, r3);
			__m128 a = _mm_add_ps(a01, a23);

			// ���O�̃��[�v�̌��ʂ��������݂܂�
			inout_prev->X = tmp_out[0];
			inout_prev->Y = tmp_out[1];
			inout_prev->Z = tmp_out[2];

			// ����̌��ʂ��X�g�A���Ă���
			_mm_storeu_ps(tmp_out, a);
			inout_prev = inout_cur;
		}

		// �Ō�̃��[�v�̌��ʂ���������
		{
			inout_prev->X = tmp_out[0];
			inout_prev->Y = tmp_out[1];
			inout_prev->Z = tmp_out[2];
		}

#else
		for (int i = 0; i < count; i++)
		{
			::Effekseer::Vector3D::Transform(
				vertexes[i].Pos,
				vertexes[i].Pos,
				mat);
		}
#endif

		for (int i = 0; i < count; i++)
		{
			auto vs = &vertexes[i];

			::Effekseer::Vector3D::Transform(
				vs->Tangent,
				vs->Tangent,
				mat);

			::Effekseer::Vector3D::Transform(
				vs->Binormal,
				vs->Binormal,
				mat);

			Effekseer::Vector3D zero;
			::Effekseer::Vector3D::Transform(
				zero,
				zero,
				mat);

			::Effekseer::Vector3D::Normal(vs->Tangent, vs->Tangent - zero);
			::Effekseer::Vector3D::Normal(vs->Binormal, vs->Binormal - zero);
		}
	}

	/*
		@class RendererImplemented
		�c�[�������̕`��@�\�����������N���X
	*/
	class RendererImplemented
		: public Renderer
		, public ::Effekseer::ReferenceObject
	{
		friend class DeviceObject;

	private:
		ID3D12Device*		m_device;
		ID3D12GraphicsCommandList* m_commandList;

		VertexBuffer*		m_vertexBuffer;
		IndexBuffer*		m_indexBuffer;
		int32_t				m_squareMaxCount;

		Shader*				m_shader;
		Shader*				m_shader_no_texture;

		Shader*				m_shader_distortion;
		Shader*				m_shader_no_texture_distortion;

		EffekseerRenderer::StandardRenderer<RendererImplemented, Shader, Vertex, VertexDistortion>* m_standardRenderer;

		::Effekseer::Vector3D	m_lightDirection;
		::Effekseer::Color		m_lightColor;
		::Effekseer::Color		m_lightAmbient;

		::Effekseer::Matrix44	m_proj;
		::Effekseer::Matrix44	m_camera;
		::Effekseer::Matrix44	m_cameraProj;

		// ���W�n
		::Effekseer::CoordinateSystem			m_coordinateSystem;

		// �`�揈���n
		::EffekseerRenderer::RenderStateBase*	m_renderState;

		::Effekseer::TextureData	m_background;

		std::set<DeviceObject*>		m_deviceObjects;

		//�X�e�[�g
		bool			m_restorationOfStates;

		
		D3D12_COMPARISON_FUNC	m_depthFunc;
		

		EffekseerRenderer::DistortingCallback* m_distortingCallback;


	public:
		/*
			�R���X�g���N�^
		*/
		RendererImplemented(int32_t squareMaxCount);

		/*
			�f�X�g���N�^
		*/
		~RendererImplemented();

		void OnLostDevice();
		void OnResetDevice();

		/*
			������
		*/

		bool Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, D3D12_COMPARISON_FUNC depthFunc);

		void Destroy();

		void SetRestorationOfStatesFlag(bool flag);

		/*
			�`��J�n
		*/

		bool BeginRendering();

		/*
			�`��I��
		*/
		bool EndRendering();

		/*
			�f�o�C�X�擾
		*/
		ID3D12Device* GetDevice();

		/*
			�R�}���h���X�g�擾
		*/
		ID3D12GraphicsCommandList* GetCommandList();

		/*
			���_�o�b�t�@�擾
		*/
		VertexBuffer* GetVertexBuffer();

		/*
			�C���f�b�N�X�o�b�t�@�擾
		*/
		IndexBuffer* GetIndexBuffer();

		/*
			�ő�`��X�v���C�g���擾
		*/
		int32_t GetSquareMaxCount() const;

		::EffekseerRenderer::RenderStateBase* GetRenderState();

		/*
			���C�g�̕������擾����
		*/

		const ::Effekseer::Vector3D& GetLightDirection() const;

		/*
			���C�g�̕�����ݒ肷��
		*/
		void SetLightDirection(::Effekseer::Vector3D& direction);

		/*
			���C�g�̐F���擾����
		*/

		const ::Effekseer::Color& GetLightColor() const;

		/**
		@brief	���C�g�̐F��ݒ肷��B
		*/
		void SetLightColor(::Effekseer::Color& color);

		/**
		@brief	���C�g�̊����̐F���擾����B
		*/
		const ::Effekseer::Color& GetLightAmbientColor() const;

		/**
		@brief	���C�g�̊����̐F��ݒ肷��B
		*/
		void SetLightAmbientColor(::Effekseer::Color& color);

		/**
		@brief	���e�s����擾����B
		*/
		const ::Effekseer::Matrix44& GetProjectionMatrix() const;

		/**
		@brief	���e�s���ݒ肷��B
		*/
		void SetProjectionMatrix(const ::Effekseer::Matrix44& mat);

		/**
		@brief	�J�����s����擾����B
		*/
		const ::Effekseer::Matrix44& GetCameraMatrix() const;

		/**
		@brief	�J�����s���ݒ肷��B
		*/
		void SetCameraMatrix(const ::Effekseer::Matrix44& mat);

		/**
		@brief	�J�����v���W�F�N�V�����s����擾����B
		*/
		::Effekseer::Matrix44& GetCameraProjectionMatrix();


		/*
			�X�v���C�g�����_���[�𐶐�����
		*/
		::Effekseer::SpriteRenderer* CreateSpriteRenderer();

		/*
			���{�������_���[�𐶐�����
		*/
		::Effekseer::RibbonRenderer* CreateRibbonRenderer();

		/*
			�����O�����_���[�𐶐�����
		*/
		::Effekseer::RingRenderer* CreateRingRenderer();

		/*
			���f�������_���[�𐶐�����
		*/
		::Effekseer::ModelRenderer* CreateModelRenderer();


		/**
		@brief	�O�Ճ����_���[�𐶐�����B
		*/
		::Effekseer::TrackRenderer* CreateTrackRenderer();

		/**
		@brief	�e�N�X�`���Ǎ��N���X�𐶐�����B
		*/
		::Effekseer::TextureLoader* CreateTextureLoader(::Effekseer::FileInterface* fileInterface = NULL);

		/**
		@brief	���f���Ǎ��N���X�𐶐�����B
		*/
		::Effekseer::ModelLoader* CreateModelLoader(::Effekseer::FileInterface* fileInterface = NULL);

		/**
		@brief	�w�i���擾����B
		*/
		Effekseer::TextureData* GetBackground() override { return &m_background; }



		/**
		@brief	�w�i��ݒ肷��B
		*/
		void SetBackground(ID3D12Resource* background);


		EffekseerRenderer::DistortingCallback* GetDistortingCallback() override;

		void SetDistortingCallback(EffekseerRenderer::DistortingCallback* callback) override;

		EffekseerRenderer::StandardRenderer<RendererImplemented, Shader, Vertex, VertexDistortion>* GetStandardRenderer() { return m_standardRenderer; }


		// ���_�o�b�t�@���Ƀf�[�^���Z�b�g����
		void SetVertexBuffer(VertexBuffer* vertexBuffer, int32_t size);
		void SetVertexBuffer(ID3D12Resource* vertexBuffer, int32_t size);
		void SetIndexBuffer(IndexBuffer* indexBuffer);
		void SetIndexBuffer(ID3D12Resource* indexBuffer);

		void SetLayout(Shader* shader);
		void DrawSprites(int32_t spriteCount, int32_t vertexOffset);
		void DrawPolygon(int32_t vertexCount, int32_t indexCount);

		void BeginShader(Shader* shader);
		void EndShader(Shader* shader);

		void SetTextures(Shader* shader, Effekseer::TextureData** textures, int32_t count);

		void ResetRenderState();

		virtual int GetRef() { return ::Effekseer::ReferenceObject::GetRef(); }
		virtual int AddRef() { return ::Effekseer::ReferenceObject::AddRef(); }
		virtual int Release() { return ::Effekseer::ReferenceObject::Release(); }

	};
}

#endif