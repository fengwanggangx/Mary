#include "Cryption.h"
#include <botan/auto_rng.h>
#include <botan/base64.h>

namespace crypto
{
	struct CAGMInfo
	{
		std::string m_strName;
		std::size_t m_nKeyLength{ 0 };
		std::size_t m_nIVLength{ 0 };
		CAGMInfo() = default;
		CAGMInfo(const std::string& strName, std::size_t key, std::size_t iv) : m_strName(strName), m_nKeyLength(key), m_nIVLength(iv) {}
		bool IsEmpty() const
		{
			return m_strName.empty() || (0 == m_nKeyLength) || (0 == m_nIVLength);
		}
	};

	const std::unordered_map<crypto::algorithm, CAGMInfo>& GetAlgorithmData()
	{
		static std::unordered_map<crypto::algorithm, CAGMInfo> s_name
		{
			{ crypto::algorithm::base64, { } },
			{ crypto::algorithm::aes_128_cbc, { "AES-128/CBC/PKCS7", 16, 16 } },
			{ crypto::algorithm::aes_256_cbc, { "AES-256/CBC/PKCS7", 32, 16 } },
			{ crypto::algorithm::aes_128_gcm, { "AES-128/GCM", 16, 12 } },
			{ crypto::algorithm::aes_256_gcm, { "AES-256/GCM", 32, 12 } },
			{ crypto::algorithm::chacha20_poly1305, { "ChaCha20Poly1305", 32, 12 } }
		};
		return s_name;
	}

	const CAGMInfo& GetAlgorithmInfo(crypto::algorithm agm)
	{
		const auto& data = GetAlgorithmData();
		const auto& mIter = data.find(agm);
		if (mIter == data.end())
		{
			static CAGMInfo info;
			return info;
		}
		return mIter->second;
	}

	CCryptor::CCryptor()
	{
	}

	CCryptor::~CCryptor()
	{
		m_crytors.clear();
	}


	std::pair<std::vector<uint8_t>, std::vector<uint8_t>> GenerateKV(crypto::algorithm agm)
	{
		const auto& info = GetAlgorithmInfo(agm);
		if (info.IsEmpty())
		{
			return {};
		}
		thread_local Botan::AutoSeeded_RNG rng;
		return { To<std::vector<uint8_t>>(rng.random_vec(info.m_nKeyLength)), To<std::vector<uint8_t>>(rng.random_vec(info.m_nIVLength)) };
	}

	Botan::AEAD_Mode* CCryptor::GetCryptor(crypto::algorithm agm, crypto::proc ty)
	{
		const auto& mIter = m_crytors.find(agm);
		if (mIter == m_crytors.end())
		{
			std::string strName = GetAlgorithmInfo(agm).m_strName;
			m_crytors[agm].first = Botan::AEAD_Mode::create(strName, Botan::Cipher_Dir::Encryption);
			m_crytors[agm].second = Botan::AEAD_Mode::create(strName, Botan::Cipher_Dir::Decryption);
		}
		return ((crypto::proc::encode == ty) ? m_crytors[agm].first : m_crytors[agm].second).get();
	}

	std::vector<uint8_t> CCryptor::Process(crypto::proc ty, const std::vector<uint8_t>& data, crypto::algorithm agm, const CParam& param)
	{
		static std::vector<uint8_t> s_empty;
		if (data.empty())
		{
			return s_empty;
		}

		if (algorithm::base64 == agm)
		{
			if (crypto::proc::encode == ty)
			{
				std::string strText = Botan::base64_encode(data);
				return To<std::vector<uint8_t>>(strText);
			}
			return To<std::vector<uint8_t>>(Botan::base64_decode(To<std::string>(data)));
		}

		if (param.IsEmpty())
		{
			return s_empty;
		}
		const CAGMInfo& info = GetAlgorithmInfo(agm);
		if (info.IsEmpty() || (param.m_key.size() != info.m_nKeyLength) || (param.m_iv.size() != info.m_nIVLength))
		{
			return s_empty;
		}

		Botan::AEAD_Mode* pCryptor = GetCryptor(agm, ty);
		if (nullptr == pCryptor)
		{
			return s_empty;
		}

		pCryptor->clear();
		pCryptor->set_key(param.m_key);
		pCryptor->start(param.m_iv);
		if (!param.m_lk.empty())
		{
			pCryptor->set_associated_data(param.m_lk);
		}

		try
		{
			std::vector<uint8_t> ecode = data;
			pCryptor->finish(ecode);
			return ecode;
		}
		catch (const Botan::Exception& e)
		{
			return s_empty;
		}
	}

	std::string CCryptor::Encrypt(const std::string& strText, crypto::algorithm agm, const CParam& param)
	{
		return To<std::string>(Encrypt(To<std::vector<uint8_t>>(strText), agm, param));
	}

	std::string CCryptor::Decrypt(const std::string& strText, crypto::algorithm agm, const CParam& param)
	{
		return To<std::string>(Decrypt(To<std::vector<uint8_t>>(strText), agm, param));
	}

	std::vector<uint8_t> CCryptor::Encrypt(const std::vector<uint8_t>& data, crypto::algorithm agm, const CParam& param)
	{
		return Process(crypto::proc::encode, data, agm, param);
	}

	std::vector<uint8_t> CCryptor::Decrypt(const std::vector<uint8_t>& data, crypto::algorithm agm, const CParam& param)
	{
		return Process(crypto::proc::decode, data, agm, param);
	}
}