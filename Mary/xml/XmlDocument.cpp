#include "Xml.h"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include <sstream> 
namespace xml
{
	XmlDocument::XmlDocument() :m_document(std::make_unique<rapidxml::xml_document<>>()) {}

	bool XmlDocument::parse(const std::string& xml)
	{
		try
		{
			m_xmlData = std::make_unique<char[]>(xml.size() + 1);
			strcpy_s(m_xmlData.get(), xml.size() + 1, xml.c_str());
			m_document->parse<0>(m_xmlData.get());
			return true;
		}
		catch (const rapidxml::parse_error& e)
		{
			return false;
		}
	}

	bool XmlDocument::loadFromFile(const std::string& filename) 
	{
		try {
			std::ifstream file(filename);
			if (!file.is_open()) {
				return false;
			}

			std::stringstream buffer;
			buffer << file.rdbuf();
			return parse(buffer.str());
		}
		catch (const std::exception& e)
		{
			return false;
		}
	}

	bool XmlDocument::saveToFile(const std::string& filename, bool formatted) const
	{
		try {
			std::ofstream file(filename);
			if (!file.is_open()) {
				return false;
			}

			if (formatted) {
				//file << rapidxml::pretty_print(m_document.get());
			}
			else {
				file << *m_document;
			}

			return true;
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

	// 转换为字符串
	std::string XmlDocument::toString(bool formatted) const
	{
		std::stringstream ss;
		if (formatted) {
			//ss << rapidxml::pretty_print(m_document.get());
		}
		else {
			ss << *m_document;
		}
		return ss.str();
	}

	// 创建根节点
	XmlNode XmlDocument::createRootNode(const std::string& name)
	{
		clear();
		std::string_view nodeName = m_document->allocate_string(name.c_str());
		rapidxml::xml_node<>* root = m_document->allocate_node(rapidxml::node_element, nodeName);
		m_document->append_node(root);
		return XmlNode(root);
	}

	// 获取根节点
	XmlNode XmlDocument::root() const
	{
		return XmlNode(m_document->first_node().get());
	}

	// 清除文档
	void XmlDocument::clear()
	{
		m_document->clear();
		m_xmlData.reset();
	}

}