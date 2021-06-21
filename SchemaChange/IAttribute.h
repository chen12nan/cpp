/*
 Copyright (c) 2018, ASML Netherlands B.V.

 THIS SOFTWARE, IN SOURCE CODE, OBJECT CODE  AND SCRIPT FORM, IS THE PROPRIETARY AND
 CONFIDENTIAL INFORMATION OF ASML NETHERLANDS B.V. (AND ITS AFFILIATES) AND IS
 PROTECTED BY U.S. AND INTERNATIONAL LAW.  ANY UNAUTHORIZED USE, COPYING AND
 DISTRIBUTION OF THIS SOFTWARE, IN SOURCE CODE, OBJECT CODE AND SCRIPT FORM, IS
 STRICTLY PROHIBITED. THIS SOFTWARE, IN SOURCE CODE, OBJECT CODE AND SCRIPT FORM IS
 PROVIDED ON AN "AS IS" BASIS WITHOUT WARRANTY, EXPRESS OR IMPLIED.  ASML
 NETHERLANDS B.V. EXPRESSLY DISCLAIMS THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 FITNESS FOR A PARTICULAR PURPOSE AND ASSUMES NO RESPONSIBILITY FOR ANY ERRORS
 THAT MAY BE INCLUDED IN THIS SOFTWARE, IN SOURCE CODE, OBJECT CODE AND IN SCRIPT
 FORM. ASML NETHERLANDS B.V. RESERVES THE RIGHT TO MAKE CHANGES TO THE SOFTWARE, IN
 SOURCE CODE, OBJECT CODE  AND SCRIPT FORM WITHOUT NOTICE.
*/

#ifndef LIB_PRO_PDB2_IATTRIBUTE_H_
#define LIB_PRO_PDB2_IATTRIBUTE_H_
#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

// #define CACHE_KEY_VTYPE 1

namespace lib {
namespace pro {
namespace pdb2 {

enum AttributeTypeItem {
    ATINone = -1,
    ATIExtrAttr,
    ATIImage,
    ATIPolygonSetOrigin,
    ATIPolygonSetSchemaChange,
    ATIPatternOrigin,
    ATIPatternSchemaChange,
    ATIPatternSetOrigin,
    ATIPatternSetSchemaChange
};

class IAttribute {
public:
    explicit IAttribute(const std::string& name);
    explicit IAttribute(const IAttribute& iAttr);
    virtual ~IAttribute(){}

    virtual void toXML(Config& top, bool keytype = false) const {
        if (m_attrPtr) {
            m_attrPtr->toXML(top, keytype);
        }
    }
    virtual void GenXML(const std::string& path, bool keytype = false) const;
    virtual bool fromXML(const Config& config, bool keytype = false);
    bool GetXML(const std::string& path, bool keytype = false);

    template <class Archive>
    void save(Archive& ar);
    template <class Archive>
    void load(Archive& ar);

    // New serializer: OPC-20926
    template <typename Archive>
    void serialize(Archive& ar);

    template <class Archive>
    void loadOldArchive(Archive& ar);
    virtual short setObjectId(const std::string& oid);
    virtual std::string getObjectId() const;
    void removeObjectId();

    std::string getName() const;
    virtual std::map<std::string, ValueType> getKeyValueTypeList() const;
    virtual ValueType hasAttr(const std::string& key) const;
    virtual short delAttr(const std::string& key, bool releaseMemory = true);
#ifndef SWIG
    virtual short setAttr(const std::string& key, const bool& value);
    virtual short setAttr(const std::string& key, const std::string& value);

    virtual short setAttr(const std::string& key, const int& value);
    virtual short setAttr(const std::string& key, const double& value);
    virtual short setAttr(const std::string& key, const Attribute& value);
    virtual short setAttr(const std::string& key, const std::vector<bool>& value);
    virtual short setAttr(const std::string& key, const std::vector<std::string>& value);
    virtual short setAttr(const std::string& key, const std::vector<int>& value) ;
    virtual short setAttr(const std::string& key, const std::vector<double>& value);
    virtual short setAttr(const std::string& key, const std::vector<Attribute*>& value);
    virtual short setAttr(const std::string& key, const std::map<std::string, std::string>& value);
    virtual short setAttr(const std::string& key, const _POINT_& value);
    virtual short setAttr(const std::string& key, const _LINE_& value) ;
    virtual short setAttr(const std::string& key, const _RING_& value);
    virtual short setAttr(const std::string& key, const _POLYGON_& value);
#endif
    short setAttr_Bool(const std::string& key, const bool& value);
    short setAttr_Str(const std::string& key, const std::string& value);
    short setAttr_Int(const std::string& key, const int& value);
    short setAttr_Dbl(const std::string& key, const double& value);
    short setAttr_Attr(const std::string& key, const Attribute& value);
    short setAttr_BoolVec(const std::string& key, const std::vector<bool>& value);
    short setAttr_StrVec(const std::string& key, const std::vector<std::string>& value);
    short setAttr_IntVec(const std::string& key, const std::vector<int>& value);
    short setAttr_DblVec(const std::string& key, const std::vector<double>& value);
    short setAttr_AttrVec(const std::string& key, const std::vector<Attribute*>& value);
    short setAttr_StrMap(const std::string& key, const std::map<std::string, std::string>& value);
    short setAttr_Point(const std::string& key, const _POINT_& value);
    short setAttr_Line(const std::string& key, const _LINE_& value);
    short setAttr_Ring(const std::string& key, const _RING_& value);
    short setAttr_Polygon(const std::string& key, const _POLYGON_& value);
#ifndef SWIG
    virtual bool* getAttr_Bool(const std::string& key) const;
    virtual std::string* getAttr_Str(const std::string& key) const;
    virtual int* getAttr_Int(const std::string& key) const;
    virtual double* getAttr_Dbl(const std::string& key) const;
#endif
    virtual bool getAttr_Bool_value(const std::string& key) const;
    virtual std::string getAttr_Str_value(const std::string& key) const;
    virtual int getAttr_Int_value(const std::string& key) const;
    virtual double getAttr_Dbl_value(const std::string& key) const;
    virtual Attribute* getAttr_Attr(const std::string& key) const;
    virtual std::vector<bool>* getAttr_BoolVec(const std::string& key) const;
    virtual std::vector<std::string>* getAttr_StrVec(const std::string& key) const;
    virtual std::vector<int>* getAttr_IntVec(const std::string& key) const;
    virtual std::vector<double>* getAttr_DblVec(const std::string& key) const;
    virtual std::vector<Attribute*>* getAttr_AttrVec(const std::string& key) const;
    typedef std::map<std::string, std::string> StrMap;
    virtual StrMap* getAttr_StrMap(const std::string& key) const;
    virtual _POINT_* getAttr_Point(const std::string& key) const;
    virtual _LINE_* getAttr_Line(const std::string& key) const;
    virtual _RING_* getAttr_Ring(const std::string& key) const;
    virtual _POLYGON_* getAttr_Polygon(const std::string& key) const;
#ifndef SWIG
    virtual AttributeTypeItem type() const = 0;
    Attribute*  getAttrPtr() const { return m_attrPtr;}
    virtual bool fromDocument(lib::infra::mongo::Document* doc);
    virtual lib::infra::mongo::Document* toDocument() const;
    virtual bool isReadOnlyKey(const std::string& key) const;
    void delID();
#endif
protected:
    Attribute* m_attrPtr;
};

}  // namespace pdb2
}  // namespace pro
}  // namespace lib

#endif  // LIB_PRO_PDB2_IATTRIBUTE_H_
