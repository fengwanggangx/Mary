#ifndef __XMLNODE_H__
#define __XMLNODE_H__

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
	class XmlAttribute;

	// XML节点类
	class XmlNode
	{
	public:
		// 构造函数
		XmlNode(rapidxml::xml_node<>* node = nullptr);

		// 判断节点是否有效
		bool isValid() const;

		// 获取节点名称
		std::string name() const;

		// 获取节点值
		std::string value() const;

		// 设置节点值
		void setValue(const std::string& value);

		// 获取第一个子节点
		XmlNode firstChild(const std::string& name = "") const;

		// 获取下一个兄弟节点
		XmlNode nextSibling(const std::string& name = "") const;

		// 获取父节点
		XmlNode parent() const;
		// 获取所有子节点
		std::vector<XmlNode> children(const std::string& name = "") const;
		// 创建子节点
		XmlNode createChild(const std::string& name, const std::string& value = "");

		// 删除子节点
		void removeChild(XmlNode& child);
		// 获取第一个属性
		XmlAttribute firstAttribute(const std::string& name = "") const;

		// 获取所有属性
		std::vector<XmlAttribute> attributes() const;

		// 添加属性
		void addAttribute(const std::string& name, const std::string& value);

		// 查找属性
		XmlAttribute findAttribute(const std::string& name) const;

		// 删除属性
		void removeAttribute(XmlAttribute& attr);

	private:
		rapidxml::xml_node<>* m_node;
		friend class XmlDocument;
	};

}

#endif
