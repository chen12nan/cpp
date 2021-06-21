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

#ifndef LIB_PRO_PDB2_DETAIL_PATTERNDBIMPL_H_
#define LIB_PRO_PDB2_DETAIL_PATTERNDBIMPL_H_
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace lib {
namespace pro {
namespace pdb2 {

class PatternDB;
class PatternQueryGenerator;

namespace detail {

class PdbVersionControl;

class PatternDBImpl : public PatternDBImplBase {
public:
    explicit PatternDBImpl(PatternDB* pdb, const std::string& path, const std::string& dbname = "default",
                           int max_duration = 3600, int retry_times = 0, int retry_perid = 30,
                           const std::string& user = "");
    ~PatternDBImpl();
    int Initialize();
    std::string getAddress() const;
    std::pair<int, int> getLastErrorCode() const;
    std::string getLastErrorMessage() const;
    void setLastErrorMessage(const std::string& msg);
    bool getId2Documents(const std::string& collName, const std::vector<std::string>& ids,
                         std::unordered_map<std::string, boost::shared_ptr<lib::infra::mongo::Document>>& documents,
                         const std::vector<std::string>& fields = std::vector<std::string>(), bool ignore = false);
    bool isConnected();
    bool isAuthorized();
    bool idExistsInCollection(const std::string& id, const std::string& collName);
    void enableReadFromSecondary(bool enable, const std::vector<std::string>& collNames = std::vector<std::string>());
    void bulkInit(bool ordered = false);
    std::string bulkExec();
    bool exportPatternSet(const std::string& outputPath, const std::string& plibRegName = "",
                          const std::string& filterStr = "");

    bool exportPatternSetList(const std::string& outputPath,
                              const std::vector<std::string>& plibNames = std::vector<std::string>(),
                              const std::string& filterStr = "");
    bool importPatternSets(const std::string& inputDataPath);
    int lock(const std::vector<std::string>& plibNames, int lockType, long waitTime = -1, int priority = -1);
    int unlock(const std::vector<std::string>& plibNames, int lockType);
    bool isEmpty();
    void addPatSetInfoIntoPattern(PatternSet* pset, Pattern& pattern);
    int getDatabaseList(std::vector<std::string>& dbList, std::string& errmsg);
    int getPatternIds(const std::string& filterStr, const std::vector<std::string>& plibNames,
                      std::vector<std::string>& patternIDs, std::string& errmsg, int skip = 0, int limit = 0);
    int getPatterns(const std::string& filterStr, const std::vector<std::string>& plibNames,
                    std::vector<Pattern*>& ptns, std::string& errmsg);
    int getPatternsByIds(const std::vector<std::string>& ids, std::vector<Pattern*>& ptns, std::string& errmsg,
                         bool checklock = true);
    int getPatternSetIdsByName(const std::string& name, const std::string& label, bool return_names,
                               std::vector<std::string>& patSetIds, std::string& errmsg);
    int getPatternSet_Id2Name(const std::string& name, const std::string& label,
                              std::vector<std::pair<std::string, std::string>>& patsetInfo, std::string& errmsg);
    int getPatternIdsInPatternSet(const std::string& lib_name, std::vector<std::string>& patIds, std::string& errmsg);
    int getPatternKeys(const std::string& plibName, std::map<std::string, lib::pro::pdb2::AttributeType>& keys,
                       std::string& errmsg);
    int fetchPatternSetByNum(const boost::function<void(std::vector<std::pair<std::string, std::string>>&)>& cb,
                             size_t num, volatile bool& stopped, const std::string& name = "",
                             const std::string& label = "");
    lib::infra::mongo::Collection* getCollectionByName(std::string collectionName, int operType, uint64_t key = 0);
    bool delPatternsByIdList(const std::vector<std::string>& pattern_id_list, PatternSet* pattern_set = NULL);
    bool enableDistributeLock() const;
    std::string getPlibNameById(const std::string& plibId);
    std::string getPlibNameByPatternId(const std::string& patnId);
    bool canReadPattern(const Pattern& pat, std::string& plibName);
    bool canWritePattern(const Pattern& pat, std::string& plibName);
    bool generateAttrQueryString(const std::string& filterStr, const std::vector<std::string>& recordIds,
                                 std::string& query) const;
    std::vector<std::string> getPatternIdsByOrder(const std::string& filter,
                                                  const std::vector<std::pair<std::string, bool>>& sortRules,
                                                  const std::string& plibName);

    bool getPatternFields(const std::string& filter, const std::vector<std::string>& fields,
                          const std::unordered_set<std::string>& patIds,
                          const std::vector<std::pair<std::string, bool>>& sortRules, volatile bool& requestStop,
                          int limit, int skip, std::vector<AttributePtr>& attrs);
    ObjectFileManagerPtr getFileStorageEngine() const;

    /*--------------------operate patternset----------------------*/
    PatternSet* createPatternSet(const std::string& name, ObjectType = OldPatternSet);
    PatternSet* getPatternSetByName(const std::string& name, bool create_if_not_exist = true);
    PatternSet* getPatternSet(const std::string& id);
    std::vector<PatternSet*> getPatternSets(const std::vector<std::string>& ids);
    std::string addPatternSet(PatternSet& pattern_set);
    bool delPatternSet(const std::string& id);
    std::string updatePatternSet(PatternSet& pattern_set);
    int getPatternCount(const std::string& plibName);

    /*--------------------operate pattern-------------------------*/
    Pattern* createPattern(const std::string& name);
    Pattern* createPattern(const Attribute& attr);
    std::string addPattern(Pattern& pattern, PatternSet* patSet);
    std::string addPattern(const std::string& patternset_name, Pattern& pattern);  // will be deleted
    Pattern* getPattern(const std::string& id, bool print_non_exist_err_msg = true);
    bool updatePattern(Pattern& pattern);
    bool delPattern(Pattern* pattern, PatternSet* pset);
    bool copyPattern(const std::vector<std::string>& patternIds, const std::string& fromPLib, const std::string& toPLib,
                     std::string* errMsg = nullptr) override;

    /*---------------------operate polygonset----------------------*/
    std::map<std::string, PolygonSet*> getPolygonSets(const std::vector<std::string>& ids);
    bool delPolygonSet(Pattern* pattern, const std::string& layer_id);
    std::vector<std::string> addPatternLayers(Pattern& pattern, const std::vector<PolygonSet*>& polygonsets);
    std::vector<std::string> addPolygonSets(Pattern& pattern, const std::vector<PolygonSet*>& polygonsets);

private:
    bool getPatternFieldsWithoutOrder(const std::string& filter, const std::vector<std::string>& fields,
                                      const std::unordered_set<std::string>& patIds, volatile bool& requestStop,
                                      int limit, int skip, std::vector<AttributePtr>& attrs);
    bool getPatternFieldsWithOrder(const std::string& filter, const std::vector<std::string>& fields,
                                   const std::unordered_set<std::string>& patIds,
                                   const std::vector<std::pair<std::string, bool>>& sortRules,
                                   volatile bool& requestStop, int limit, std::vector<AttributePtr>& attrs);

    std::vector<boost::shared_ptr<infra::mongo::Document>>
    findDocuments(infra::mongo::CollectionPtr coll, const std::string& query, const std::vector<std::string>& fields,
                  int limit, const std::vector<std::pair<std::string, bool>>& sortRules, volatile bool& requestStop);
    std::string bulkAddPolygonSet(Pattern& pattern, PolygonSet* polygonset);
    int getPatternIdsInPatternSet_(const std::string& lib_name, std::vector<std::string>& pids, std::string& err_msg);
    Pattern* getPattern_(const std::string& id, int retry, std::string& err_msg, bool print_non_exist_err_msg);

    short delPattern_(Pattern* pattern, PatternSet* pset, std::string& err_msg);

    int getPatternSetList_(const std::string& name, const std::string& label, int retry,
                           std::vector<std::pair<std::string, std::string>>& res_list, std::string& err_msg,
                           const std::vector<std::string>& ids);

    std::string addPatternSet_(PatternSet& patternset, int retry, std::string& err_msg);

    PatternSet* getPatternSet_(const std::string& id, int retry, std::string& err_msg,
                               bool fetch_subpatset_info = true);

    short delPatternSet_(const std::string& id, int retry, std::string& err_msg);

    std::string updatePatternSet_(PatternSet& pattern_set, int retry, std::string& err_msg);

    std::vector<std::string> generatePatternQueryString_(const std::string& filterStr,
                                                         const std::vector<std::string>& plibNames,
                                                         int fetchNum = 10000);

    std::vector<std::string> generatePatternQueryString_(const std::string& filter,
                                                         const std::unordered_set<std::string>& patIds,
                                                         int fetchNum = 10000);

    std::vector<Pattern*>
    handlePatternDocuments_(boost::shared_ptr<std::vector<boost::shared_ptr<infra::mongo::Document>>> docs);
    std::vector<PolygonSet*>
    handlePolygonsetDocuments_(boost::shared_ptr<std::vector<boost::shared_ptr<infra::mongo::Document>>> docs);

    typedef std::vector<Attribute*>::const_iterator VecIter;
    std::vector<boost::shared_ptr<infra::mongo::Document>> handleAttributesFunc_(VecIter begin, VecIter end);

    typedef std::vector<tpool::FutureTask<std::vector<boost::shared_ptr<lib::infra::mongo::Document>>>::Ptr>
        FutureTasks;
    void getFutureTasks(const std::vector<Attribute*>& attrs, FutureTasks& tasks);
    void addDocs2Collection(FutureTasks& tasks, const std::string& collName);

    bool releaseMemory();

    bool doExportPatternSets_(const std::string& outputPath, const std::vector<std::string>& plibIDs,
                              const std::string& filterStr);
    bool importPatternSet_(PatternSet& patternSet, std::string& warnings);
    std::string importPattern_(Pattern& pattern, const PatternSet& plib);

    void removePatternFromPatSet_(Pattern* pattern, PatternSet* pset);

    SubPatternSet* getSubPatternSet_(const std::string& id, int retry, std::string& err_msg);
    int getPatternSetIdsByName_(const std::string& name, const std::string& label, bool return_names,
                                std::vector<std::string>& patSetIds);

    int getPatternsByIds_(const std::vector<std::string>& ids, std::vector<Pattern*>& ptns, bool checklock);
    int getPolygonSets_(const std::vector<std::string>& ids, std::map<std::string, PolygonSet*>& polys);
    int getDocumentList(const std::string& collName, const std::vector<std::string>& ids,
                        lib::infra::mongo::DocumentList*& list,
                        const std::vector<std::string>& fields = std::vector<std::string>(), bool ignore = false);

    int getPatterns_(const std::string& filterStr, const std::vector<std::string>& plibNames,
                     std::vector<Pattern*>& ptns);
    int getPatternIds_(const std::string& filterStr, const std::vector<std::string>& plibNames,
                       std::vector<std::string>& ids, int skip = 0, int limit = 0);
    int getPatternSets_(const std::vector<std::string>& ids, std::vector<PatternSet*>& ptnsets,
                        bool fetch_subpatset_info = true);
    std::string addPatternToPatSet_(PatternSet* patternset, Pattern& pattern);
    void addPatSetInfoIntoPattern_(PatternSet* pset, Pattern& pattern);
    int fetchPatternSetByNum_(const boost::function<void(std::vector<std::pair<std::string, std::string>>&)>& cb,
                              size_t num, volatile bool& stopped, const std::string& name, const std::string& label);
    void getPatsetInfoByPattern_(Pattern* pattern, PatternSet*& patSet);

    bool connectToPatternDB();

    static bool isValidDbPath_(const std::string& path);

    bool canReadPlibByName(const std::string& plibName) const;
    bool canWritePlibByName(const std::string& plibName) const;

    bool canReadPlibByNames(const std::vector<std::string>& plibNames) const;
    bool canWritePlibByNames(const std::vector<std::string>& plibNames) const;

    bool canReadPlib(const PatternSet& pSet) const;
    bool canWritePlib(const PatternSet& pSet) const;

    bool canAccessPlibById(const std::string& id, bool write, std::string& plibName);

    bool canReadPlibByIds(const std::vector<std::string>& plibIds);
    bool canWritePlibByIds(const std::vector<std::string>& plibIds);

    std::string getLockTypeStr(int type) const;

    int fetchPatternsByQueries(const std::vector<std::string>& queries, std::vector<Pattern*>& ptns,
                               std::set<std::string>* plibIds = NULL);

    static bool checkLicense();
    void stopFetchPatternSetByNum_(std::vector<std::pair<std::string, std::string>>& res_list,
                                   const boost::function<void(std::vector<std::pair<std::string, std::string>>&)>& cb,
                                   lib::infra::mongo::DocumentList*& list);
    /**
     * @brief Set the db operation retry parameters.
     * @param op_retry_times: The retry times(not include the first run).
     * @param op_retry_interval: each retry have a interval second
     */
    int setOpRetryParam(int op_retry_times, int op_retry_interval);

    std::vector<std::string> generateQueryString(const std::string& filter, const std::string& plibName, bool sort);

private:
    std::string m_dbpath;
    std::string m_dbname;
    bool m_db_authorized;
    bool m_db_writable;
    std::string m_address;
    // If one PDB collection operation is failed and it consumes time is less than MAX_RETRY_TIME,
    // it will retry the operation. If it is 0, means there is no retry.
    int m_max_duration;
    int m_db_retry_times;  // read/write db retry times
    int m_db_retry_perid;  // read/write db retry interval

    boost::scoped_ptr<PatternQueryGenerator> m_queryGenerator;
    boost::scoped_ptr<lib::infra::mongo::Database> m_db;

    bool m_bulkPatternOrdered;
    bool m_bulkPolygonSetOrdered;
    bool m_allowChangeOrdered;
    bool m_bulk_enabled;
    boost::scoped_ptr<lib::infra::commonutil::Settings> m_settings;
    bool invalidBulkUsage(const std::string& func);

    std::set<PatternSet*> m_toBeUpdatePatternSets;
    std::map<std::string, PatternSet*> m_id2Plibs;
    // std::vector<boost::shared_ptr<Attribute> > m_toBeInsertPatterns;
    std::vector<Attribute*> m_toBeInsertPolygonsets;

    std::string m_user;
    mutable std::map<std::string, std::string> m_patnId2PlibId;
    mutable std::map<std::string, std::string> m_plibId2Name;
    mutable char m_enableLock;
    std::unique_ptr<detail::PdbVersionControl> m_pdbVersionControl;
    PatternDB* m_pdb;
    ObjectFileManagerPtr m_fileStorageEngine;
};

}  // namespace detail
}  // namespace pdb2
}  // namespace pro
}  // namespace lib

#endif  // LIB_PRO_PDB2_DETAIL_PATTERNDBIMPL_H_
