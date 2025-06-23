#include "XmlAttribute.h"

namespace xml
{

	XmlAttribute::XmlAttribute(rapidxml::xml_attribute<>* attr) : m_attr(attr) {}

	// 判断属性是否有效
	bool XmlAttribute::isValid() const { return m_attr != nullptr; }

	// 获取属性名称
	std::string XmlAttribute::name() const {
		if (!m_attr) return "";
		return m_attr->name().data();
	}

	// 获取属性值
	std::string XmlAttribute::value() const {
		if (!m_attr) return "";
		return m_attr->value().data();
	}

	// 设置属性值
	void XmlAttribute::setValue(const std::string& value) {
		if (m_attr) {
			std::string_view newValue = m_attr->document()->allocate_string(value.c_str());
			m_attr->value(newValue);
		}
	}

}