#ifndef __XMLATTRIBUTE_H__
#define __XMLATTRIBUTE_H__


#include <rapidxml_print.hpp>


namespace xml
{

	class XmlNode;

	// XML属性类
	class XmlAttribute
	{
		friend class XmlNode;
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
	};
}

#endif
