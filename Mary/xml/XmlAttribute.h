#ifndef __XMLATTRIBUTE_H__
#define __XMLATTRIBUTE_H__

#include <rapidxml.hpp>
#include <rapidxml_print.hpp>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <fstream>

namespace xml
{
	// 前向声明
	class XmlDocument;
	class XmlNode;

	// XML属性类
	class XmlAttribute
	{
	public:
		// 构造函数
		XmlAttribute(rapidxml::xml_attribute<>* attr = nullptr);

		// 判断属性是否有效
		bool isValid() const;

		// 获取属性名称
		std::string name() const;

		// 获取属性值
		std::string value() const;
		// 设置属性值
		void setValue(const std::string& value);
	private:
		rapidxml::xml_attribute<>* m_attr;
		friend class XmlNode;
	};

	// XML文档类
	class XmlDocument
	{
	public:
		// 构造函数
		XmlDocument();

		// 禁止拷贝
		XmlDocument(const XmlDocument&) = delete;
		XmlDocument& operator=(const XmlDocument&) = delete;

		// 移动构造函数和赋值运算符
		XmlDocument(XmlDocument&& other) noexcept = default;
		XmlDocument& operator=(XmlDocument&& other) noexcept = default;

		// 解析XML字符串
		bool parse(const std::string& xml);
		// 从文件加载XML
		bool loadFromFile(const std::string& filename);
		// 保存XML到文件
		bool saveToFile(const std::string& filename, bool formatted = true) const;

		// 转换为字符串
		std::string toString(bool formatted = true) const;

		// 创建根节点
		XmlNode createRootNode(const std::string& name);
		// 获取根节点
		XmlNode root() const;
		// 清除文档
		void clear();
	private:
		std::unique_ptr<rapidxml::xml_document<>> m_document;
		std::unique_ptr<char[]> m_xmlData;
	};

}

#endif
