#include "XmlNode.h"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include <sstream> 
#include "XmlAttribute.h"
namespace xml
{
	// XmlNode方法实现
	XmlAttribute XmlNode::firstAttribute(const std::string& name) const 
	{
		if (!m_node) return XmlAttribute();
		return name.empty() ?
			XmlAttribute(m_node->first_attribute().get()) :
			XmlAttribute(m_node->first_attribute(name.c_str()).get());
	}

	std::vector<XmlAttribute> XmlNode::attributes() const 
	{
		std::vector<XmlAttribute> result;
		if (!m_node) return result;

		rapidxml::xml_attribute<>* attr = m_node->first_attribute().get();
		while (attr) {
			result.emplace_back(attr);
			attr = attr->next_attribute().get();
		}

		return result;
	}



	XmlNode::XmlNode(rapidxml::xml_node<>* node) : m_node(node) {}

	// 判断节点是否有效
	bool XmlNode::isValid() const { return m_node != nullptr; }

	// 获取节点名称
	std::string XmlNode::name() const
	{
		if (!m_node) return "";
		return m_node->name().data();
	}

	// 获取节点值
	std::string XmlNode::value() const {
		if (!m_node) return "";
		return m_node->value().data();
	}

	// 设置节点值
	void XmlNode::setValue(const std::string& value) {
		if (m_node) {
			std::string_view newValue = m_node->document()->allocate_string(value.c_str());
			m_node->value(newValue);
		}
	}

	// 获取第一个子节点
	XmlNode XmlNode::firstChild(const std::string& name) const {
		if (!m_node) return XmlNode();
		return name.empty() ?
			XmlNode(m_node->first_node().get()) :
			XmlNode(m_node->first_node(name.c_str()).get());
	}

	// 获取下一个兄弟节点
	XmlNode XmlNode::nextSibling(const std::string& name) const {
		if (!m_node) return XmlNode();
		return name.empty() ?
			XmlNode(m_node->next_sibling().get()) :
			XmlNode(m_node->next_sibling(name.c_str()).get());
	}

	// 获取父节点
	XmlNode XmlNode::parent() const {
		if (!m_node || !m_node->parent()) return XmlNode();
		return XmlNode(m_node->parent().get());
	}

	// 获取所有子节点
	std::vector<XmlNode> XmlNode::children(const std::string& name) const
	{
		std::vector<XmlNode> result;
		if (!m_node) return result;

		rapidxml::xml_node<>* child = name.empty() ?
			m_node->first_node().get() :
			m_node->first_node(name.c_str()).get();

		while (child) {
			result.emplace_back(child);
			child = name.empty() ? child->next_sibling().get() : child->next_sibling(name.c_str()).get();
		}

		return result;
	}

	// 创建子节点
	XmlNode XmlNode::createChild(const std::string& name, const std::string& value) {
		if (!m_node) return XmlNode();

		std::string_view nodeName = m_node->document()->allocate_string(name.c_str());
		rapidxml::xml_node<>* newNode = m_node->document()->allocate_node(rapidxml::node_element, nodeName);

		if (!value.empty()) {
			std::string_view nodeValue = m_node->document()->allocate_string(value.c_str());
			newNode->value(nodeValue);
		}

		m_node->append_node(newNode);
		return XmlNode(newNode);
	}

	// 删除子节点
	void XmlNode::removeChild(XmlNode& child) {
		if (m_node && child.m_node && child.m_node->parent() == m_node) {
			m_node->remove_node(child.m_node);
			child.m_node = nullptr;
		}
	}


	inline void XmlNode::addAttribute(const std::string& name, const std::string& value) {
		if (!m_node) return;

		std::string_view attrName = m_node->document()->allocate_string(name.c_str());
		std::string_view attrValue = m_node->document()->allocate_string(value.c_str());

		rapidxml::xml_attribute<>* attr = m_node->document()->allocate_attribute(attrName, attrValue);
		m_node->append_attribute(attr);
	}

	inline XmlAttribute XmlNode::findAttribute(const std::string& name) const {
		if (!m_node) return XmlAttribute();

		rapidxml::xml_attribute<>* attr = m_node->first_attribute(name.c_str()).get();
		return XmlAttribute(attr);
	}

	inline void XmlNode::removeAttribute(XmlAttribute& attr) {
		if (m_node && attr.m_attr && attr.m_attr->parent() == m_node) {
			m_node->remove_attribute(attr.m_attr);
			attr.m_attr = nullptr;
		}
	}

}