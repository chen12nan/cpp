/*
 Copyright (c) 2020, ASML Netherlands B.V.

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

#ifndef LIB_PRO_PDB2_PATTERN_SET_H_
#define LIB_PRO_PDB2_PATTERN_SET_H_

#include <map>
#include <mutex>
#include <set>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "lib/pro/pdb2/attribute.h"
#include "lib/pro/pdb2/pattern.h"
#include "lib/pro/pdb2/subpattern_set.h"

namespace lib {
namespace pro {
namespace pdb2 {

namespace detail {
class PatternDBImpl;
}
class PatternDB;

class PatternSet : public IAttribute {
public:
    PatternSet(const std::string& name, PatternDB* pdb, int max_retry);
    virtual ~PatternSet();
    std::map<std::string, std::string> getPatternIds();
    size_t size();
#ifndef SWIG
    virtual bool fromDocument(lib::infra::mongo::Document* doc);
    friend class lib::pro::pdb2::detail::PatternDBImpl;
    template <typename Archive>
    void serialize(Archive& ar);
    short setAttr(const std::string& key, const std::string& value) override;
    short delAttr(const std::string& key, bool releasememory = true) override;
    int getSubPatternSetList(std::vector<std::string>& subpatsetid_list);
    std::map<std::string, std::string> getPatternIdsBySubPatternSet(const std::string& subpatset_id);
    PatternDB* getPDB();
    int initSubPatternSets(std::string& err_msg, volatile bool& stopped);
    std::string generateSubPlib(bool flushToDB = true);
    int updateToPDB();
    short attachPattern(const std::string& id, bool checkConsistent = true, const std::string& subPlibId = "");
    std::string getLastSubPlibId() const;
    int getPatternCapacity(const std::string& subPlibId) const;
    static std::set<std::string> getReadOnlyKeys();
    std::vector<std::string> getOldPatternIds() const;
    std::string getQueryStrOfOldPatternIds() const;
    SubPatternSetPtr getSubPlib(const std::string& subPlibId);

    //--------------------------------- new api
    PatternPtr createPattern(const std:string& name);
    PatternPtr createPattern(const IAttribute& iAttr);
    std::string addPattern(PatternPtr pattern);
    PatternPtr getPattern(const std::string& id);
    bool updatePattern(PatternPtr& patn);
    bool delPattern(const std::vector<std::string>& id);

    bool copyPatternFrom(const std::vector<std::string>& pids, const std::string& originPLib);
    std::vector<PatternPtr> getPatterns(const std::string& filterStr);
    std::vector<PatternPtr> getPatternsByIds(const std::vector<std::string> ids);
    std::vector<std::string> getPatternIds(const std::string& filterStr);
    //-----------------------------------------
#endif
// the following function will be moved ot PatternSetOriginImpl.
private:
    short attachPattern(const std::string& id, const std::string& name, bool checkConsistent = true,
                        const std::string& subPlibId = "");
    short detachPattern(const std::string& id, bool checkConsistent = true);
    short addNewSubPatternSet(std::string* subPlibId = nullptr, bool flush = true);
    int getNewSubPatternSetName(std::string& name);
    SubPatternSetPtr handleDocumentsFunc_(boost::shared_ptr<infra::mongo::Document> doc);
    int addSubPatSetToDB(SubPatternSetPtr subpatset, std::string& subpatset_id);
    int addSubPatSetIdToPatternSet(const std::string& subpatset_id);
    size_t getSelfPatternSize();
    virtual bool isReadOnlyKey(const std::string& key) const;
    short getAvailableSubPatternSet();
    std::string addPatSetToDB();
    int addPatInSubPatSetDB();
    int delPatInSubPatSetDB();
    int addSubPatSetInPatSetDB();
    int updatePatSetAttrDB(int op_type);
    int refreshLastSubPatSet();

    template <typename T>
    void addAttributeIfNotExist(const std::string& attrName, int attrType, const T& defaultVal);

private:
    PatternDB* m_pdb;
    int MAX_RETRY_TIME;
    std::vector<SubPatternSetPtr> m_subpatsets;
    bool m_init_subpatsets;
    static std::set<std::string> m_readonlyKeys;
    std::vector<std::string> m_add_subpatsets;
    std::map<std::string, std::map<std::string, std::string>> m_addPats;
    std::map<std::string, std::map<std::string, std::string>> m_delPats;
    std::map<std::string, std::string> m_update_attrs;
    std::map<std::string, std::string> m_del_attrs;
    static std::mutex m_subPlibSizeMutex;
    static std::map<std::string, int> m_subPlibId2patnum;
};

using PatternSetPtr = boost::shared_ptr<PatternSet>;
}  // namespace pdb2
}  // namespace pro
}  // namespace lib

#endif  // LIB_PRO_PDB2_PATTERN_SET_H_
