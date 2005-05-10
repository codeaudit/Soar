#include "ilobject.h"
#include <iostream>
#include <cassert>

using std::string;
using std::vector;
using std::cout; using std::endl;
using std::ostream;

extern void pause();
/******************************************************************************
* ILObject Class Function Definitions
*
*
******************************************************************************
*/

InputLinkObject::InputLinkObject()
{
	m_updateFrequency = UPDATE_NEVER;
}

/* Destructor
 *
 */  //TODO find out why this doesn't work correctly.  seems so simple
InputLinkObject::~InputLinkObject()
{
	m_elementTypes.clear();
}

std::ostream& operator << (std::ostream& stream, InputLinkObject& obj)
{
	stream << "Insertion operator printing an input link object..." << endl;
	stream << "\tParent: \t" << obj.m_parentId << endl;
	stream << "\tAttribute: \t" << obj.m_attribName << endl;
	for(typesIterator tItr = obj.m_elementTypes.begin(); tItr != obj.m_elementTypes.end(); ++tItr)
	{
		stream << "\tType: \t\t";
		//stream << obj.printType(stream, *tItr) << endl;
		switch(*tItr)
		{
		case ELEMENT_FLOAT:
			stream << "float element";
			break;
		case ELEMENT_INT:
			stream << "int element";
			break;
		case ELEMENT_ID:
			stream << "identifier";
			break;
		case ELEMENT_STRING:
			stream << "string element";
			break;
		case ELEMENT_TYPE_TBD:
			stream << k_TBD;
			break;
		default:
			stream << "BAAAAD things have happened";
			assert(false);
			break;
		}
		stream << endl;
	}
	stream << "\tValue: \t\t";
	//stream << obj.printValue(stream) << endl;
	switch(obj.m_curType)
	{
		case ELEMENT_STRING:
			stream << (obj.m_value.s);
			break;
		case ELEMENT_FLOAT:
			stream << (obj.m_value.f);
			break;
		case ELEMENT_ID:
			stream << (obj.m_value.id);
			break;
		case ELEMENT_INT:
			stream << (obj.m_value.i);
			break;
		case ELEMENT_TYPE_TBD:
			stream << k_TBD;
			break;
		default:
			stream << "Wow...how did this happen?" << endl;
			assert(false);
			break;
	}
	stream << endl;
	stream << "\tFrequency: \t" << obj.getFrequencyAsString() << endl;

	stream << endl;
	return stream;
}

/*
//TODO comment  //TODO consider printing the const values instead of these
ostream& InputLinkObject::printType(ostream& stream, eElementType type)
{
	switch(type)
	{
		case ELEMENT_FLOAT:
			stream << "float element";
			break;
		case ELEMENT_INT:
			stream << "int element";
			break;
		case ELEMENT_ID:
			stream << "identifier";
			break;
		case ELEMENT_STRING:
			stream << "string element";
			break;
		default:
			stream << "BAAAAD things have happened";
			assert(false);
			break;
	}

	return stream;
}*/

const string typeToString(eElementType type)
{
	switch(type)
	{
		case ELEMENT_FLOAT:
			return k_floatString;
			break;
		case ELEMENT_INT:
			return k_intString;
			break;
		case ELEMENT_ID:
			return k_idString;
			break;
		case ELEMENT_STRING:
			return k_stringString;
			break;
		default:
			return "BAAAAD things have happened";
			assert(false);
			break;
	}
}
//TODO comment
/*ostream& InputLinkObject::printValue(ostream& stream)
{
	switch(m_curType)
	{
		case ELEMENT_STRING:
			assert(m_value.s);
			stream << *(m_value.s);
			break;
		case ELEMENT_FLOAT:
			assert (m_value.f);
			stream << *(m_value.f);
			break;
		case ELEMENT_ID:
			assert(m_value.id);
			stream << *(m_value.id);
			break;
		case ELEMENT_INT:
			assert(m_value.i);
			stream << *(m_value.i);
			break;
		default:
			stream << "Wow...how did this happen?" << endl;
			assert(false);
			break;
	}
	return stream;
}*/


//TODO comment
void InputLinkObject::setType(string& inValue)
{
	m_curType = stringToType(inValue);
}

void InputLinkObject::setType()
{
	assert(!m_elementTypes.empty());
	m_curType = m_elementTypes[0];
}

void InputLinkObject::setType(const std::string& inValue)
{
	m_curType = stringToType(inValue);
}

//TODO comment
void InputLinkObject::setStartValue(string& inValue)
{
	//sanity checks
	assert(!m_elementTypes.empty());

	switch(m_elementTypes[0])
	{
		case ELEMENT_STRING:
			assert(m_value.s = "");
			m_value.s = inValue;
			break;
		case ELEMENT_INT:
			assert(m_value.i = "");
			m_value.i = inValue; //atoi(inValue.c_str());
			break;
		case ELEMENT_FLOAT:
			assert(m_value.f = "");		
			m_value.f = inValue; //atof(inValue.c_str());
			break;
		case ELEMENT_ID:
			assert(m_value.id = "");		
			m_value.id = inValue;
			break;
		default:
			assert(!"got unexpected enum type....");
			break;
	}
}

void InputLinkObject::setUpdateValue(std::string& inValue)
{
	switch(m_curType)
	{
		case ELEMENT_INT:
		case ELEMENT_FLOAT:
		case ELEMENT_STRING:
			m_updateValue = inValue;
			break;
		case ELEMENT_ID:
			//TODO think about the case where an wme points to one id, and later to another.
			//isn't this conceivable?  and in that case, the ID could need to be updated
			assert(!"ID type should never receive an update value");
			break;
		default:
			break;			
	}
}

void InputLinkObject::setUpdateFrequency(string& inValue)
{
	if(inValue == k_onChangeString)
		m_updateFrequency = UPDATE_ON_CHANGE;
	else if(inValue == k_conditionString)
		m_updateFrequency = UPDATE_ON_CONDITION;
	else if(inValue == k_cycleString)
		m_updateFrequency = UPDATE_EVERY_CYCLE;
	else
		cout << "Got some unexpected freqency:>" << inValue << "<" << endl;
}

string InputLinkObject::getFrequencyAsString() const
{
	switch(m_updateFrequency)
	{
		case UPDATE_EVERY_CYCLE:
			return "every cycle";
			break;
		case UPDATE_NEVER:
			return "never";
			break;
		case UPDATE_ON_CHANGE:
			return "on change";
			break;
		case UPDATE_ON_CONDITION:
			return "on condition";
			break;
		default:
			assert("object acquired some goofy, unexpected frequency" == 0);
			return "foo";//won't execute
			break;
	}
}
	
void InputLinkObject::setUpdateCondition(string& inValue){	m_updateCondition = inValue;} 

//does a case-insensitive string comparison, mapping strings to enums
eElementType stringToType(const string& source)
{
	if(!stricmp(source.c_str(), k_intString.c_str()))
		return ELEMENT_INT;
	else if(!stricmp(source.c_str(), k_stringString.c_str()))
		return ELEMENT_STRING;
	else if(!stricmp(source.c_str(), k_floatString.c_str()))
		return ELEMENT_FLOAT;
	else if(!stricmp(source.c_str(), k_idString.c_str()))
		return ELEMENT_ID;
	else if(!stricmp(source.c_str(), k_TBD.c_str()))
		return ELEMENT_TYPE_TBD;
	else
	{
		cout << "stringToType: bad type found: " << source << std::endl;
		pause();
		exit(-1);
	}
}