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

#ifndef LIB_PRO_PDB2_ATTRIBUTE_H_
#define LIB_PRO_PDB2_ATTRIBUTE_H_
#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

// #define CACHE_KEY_VTYPE 1

namespace lib {
namespace pro {
namespace pdb2 {

class AttrMapImp {}

typedef AttrMapImp<std::map<std::string, bool>> attr_map_bool;
typedef AttrMapImp<map_imp<std::string>::type> attr_map_str;
typedef AttrMapImp<std::map<std::string, int>> attr_map_int;
typedef AttrMapImp<std::map<std::string, double>> attr_map_dbl;
class Attribute;
typedef AttrMapImp<map_imp<Attribute*>::type> attr_map_attr;
typedef AttrMapImp<std::map<std::string, std::vector<bool>>> attr_map_bool_vec;
typedef AttrMapImp<std::map<std::string, std::vector<std::string>>> attr_map_str_vec;
typedef AttrMapImp<std::map<std::string, std::vector<int>>> attr_map_int_vec;
typedef AttrMapImp<std::map<std::string, std::vector<double>>> attr_map_dbl_vec;
typedef AttrMapImp<std::map<std::string, std::vector<Attribute*>>> attr_map_attr_vec;
typedef AttrMapImp<std::map<std::string, _POINT_>> attr_map_point;
typedef AttrMapImp<std::map<std::string, _LINE_>> attr_map_line;
typedef AttrMapImp<std::map<std::string, _RING_>> attr_map_ring;
typedef AttrMapImp<map_imp<_POLYGON_>::type> attr_map_polygon;
typedef AttrMapImp<std::map<std::string, std::map<std::string, std::string>>> attr_map_reg_table;
static const double ATTR_VERSION = 20.02;
extern const char* EXTRA_ATTR_IDS;

class Attribute {
public:
    explicit Attribute(const std::string& name);
    Attribute() : is_sub_attr(true)
    {
    }
    Attribute(const Attribute& that);
    Attribute& operator=(const Attribute& that);
    virtual ~Attribute();

#ifndef SWIG
    virtual void toXML(Config& top, bool keytype = false) const;
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

    bool operator==(const Attribute& o) const;
    bool operator!=(const Attribute& o) const;

    virtual short setObjectId(const std::string& oid);
    virtual std::string getObjectId() const;
    void removeObjectId();
#endif
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
    virtual short setAttr(const std::string& key, const std::vector<int>& value);
    virtual short setAttr(const std::string& key, const std::vector<double>& value);
    virtual short setAttr(const std::string& key, const std::vector<Attribute*>& value);
    virtual short setAttr(const std::string& key, const std::map<std::string, std::string>& value);
    virtual short setAttr(const std::string& key, const _POINT_& value);
    virtual short setAttr(const std::string& key, const _LINE_& value);
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
    virtual bool fromDocument(lib::infra::mongo::Document* doc);
    virtual lib::infra::mongo::Document* toDocument() const;
    void setSub(bool sub = true);
    bool isSub() const;
    static Int64 toInt64ByDBU(double value, double dbu);
    virtual bool isReadOnlyKey(const std::string& key) const；
    void copy(const Attribute& that);
    void delID();
#endif
protected:
    void releaseAttributes();
    virtual std::vector<std::string> get_reserved_keys() const;
    void addKey2ValueType(const std::string& key, ValueType type);
    void removeKey2ValueType(const std::string& key);
    std::vector<std::pair<std::string, AttributeVariantPtr>>
    handleSubAttribute(const std::vector<std::pair<std::string, lib::infra::mongo::Document*>> pairs);
#ifdef CACHE_KEY_VTYPE
    std::map<std::string, ValueType> m_key_vtype;
#endif
    // mutable std::vector<AttrMap*> m_attrmaps;
    mutable std::map<std::string, AttributeVariantPtr> m_attrMap;
    typedef std::map<std::string, AttributeVariantPtr>::iterator MapIter;
    typedef std::map<std::string, AttributeVariantPtr>::const_iterator ConstMapIter;

    short doSetAttr(const std::string& key, const bool& value);
    short doSetAttr(const std::string& key, const std::string& value);
    short doSetAttr(const std::string& key, const char* value);
    short doSetAttr(const std::string& key, const int& value);
    short doSetAttr(const std::string& key, const double& value);
    short doSetAttr(const std::string& key, const Attribute& value);
    short doSetAttr(const std::string& key, const std::vector<bool>& value);
    short doSetAttr(const std::string& key, const std::vector<std::string>& value);
    short doSetAttr(const std::string& key, const std::vector<int>& value);
    short doSetAttr(const std::string& key, const std::vector<double>& value);
    short doSetAttr(const std::string& key, const std::vector<Attribute*>& value, bool deepCopy = true);
    short doSetAttr(const std::string& key, const std::map<std::string, std::string>& value);
    short doSetAttr(const std::string& key, const _POINT_& value);
    short doSetAttr(const std::string& key, const _LINE_& value);
    short doSetAttr(const std::string& key, const _RING_& value);
    short doSetAttr(const std::string& key, const _POLYGON_& value);
    short doDelAttr(const std::string& key, bool releaseMemory = true);

    // Just for StrMap value type
    bool setMapAttr(const std::string& attrKey, const std::string& mapKey, const std::string& mapValue);
    bool delMapAttr(const std::string& attrKey, const std::string& mapKey);
    bool hasMapAttr(const std::string& attrKey, const std::string& mapKey) const;

private:
    bool isValidKey(const std::string& key) const;
    short doSetAttr(const std::string& key, Attribute* value);

private:
    bool is_sub_attr;
    friend class IAttribute;
};

using AttributePtr = std::shared_ptr<Attribute>;

}  // namespace pdb2
}  // namespace pro
}  // namespace lib

#endif  // LIB_PRO_PDB2_ATTRIBUTE_H_
