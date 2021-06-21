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

#ifndef LIB_PRO_PDB2_PATTERN_H_
#define LIB_PRO_PDB2_PATTERN_H_
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <iostream>

#include "PolygonSet.h"
namespace lib {
namespace pro {
namespace pdb2 {

namespace detail {
class PatternDBImpl;
}

class PatternDB;
class ExtraAttr;
using ExtraAttrPtr = ExtraAttr*;
using RecordPtr = ExtraAttrPtr;
#define DEFAULT_DBU_VALUE 0.001
static const std::string ExtraAttr_Coll_Name = "ExtraAttrs";
static const std::string Object_Files = "object_files";
static const std::string File_Uri = "uri";

class Pattern : public IAttribute {
public:
#ifndef SWIG
    enum polygonSet_action_type { UNKNOWN_ACTION_TYPE = 0, ATTACH_POLYGON_SET, DETTACH_POLYGON_SET };
    enum varied_dbu_type { UNKNOWN_DBU_TYPE = 0, NEW_DBU_TYPE, SAME_DBU_TYPE, DIFF_DBU_TYPE };
    // mark pattern generated type.
    enum pattern_source_type { UNKNOWN_SOURCE_TYPE = 0, SOURCE_BASIC_TYPE, SOURCE_IMPORT_TYPE };
    Pattern(const std::string& name, PatternDB* pdb, int max_retry);
    Pattern(const IAttribute& attr, PatternDB* pdb, int max_retry);

    Pattern() : m_pdb(nullptr), MAX_RETRY_TIME(0), m_source_type(UNKNOWN_SOURCE_TYPE);
    virtual ~Pattern();

    // --------------  implement in Pattern class---------------
    PatternDB* getPDB() const;
    std::string getPatternSetId() const;
    void delCommonBBox();
    void clearContentExceptID();
    void clearEditableContent();
    bool getMinBox(Int64& left, Int64& botttom, Int64& right, Int64& top) const;
    short setAllAngleFlag(bool allAngle);

    std::string
    addObjectFile(const std::string& name, const std::string& filePath,
                  const std::map<std::string, std::string>& properties = std::map<std::string, std::string>());
    std::string getObjectFileInfo(const std::string& name, std::map<std::string, std::string>* properties = nullptr);
    bool attachObjectFile(const std::string& name, const std::string& fileUri,
                          const std::map<std::string, std::string>& properties = std::map<std::string, std::string>());
    std::string getObjectFileContent(const std::string& name, std::map<std::string, std::string>* properties = nullptr);
    std::string getObjectFilePath(const std::string& name, const std::string& suffix = "",
                                  std::map<std::string, std::string>* properties = nullptr);
    bool detachObjectFile(const std::string& name);
    bool addObjectFileProperty(const std::string& name, const std::map<std::string, std::string>& properties);
    bool removeObjectFileProperty(const std::string& name, const std::vector<std::string>& propertyKeys);


    ExtraAttrPtr createExtraAttr(const std::string& attrName) const;

    // ------------------need to get ----------
    void setPlibId(const std::string& plibId, const std::string& subPlibId);
    std::string addExtraAttr(const std::string& group, ExtraAttrPtr attr);
    bool deleteExtraAttrs(const std::string& group, const std::string& filterStr = "");
    std::vector<ExtraAttrPtr> getExtraAttrs(const std::string& group, const std::string& filterStr = "") const;
    updateExtraAttr(const std::string& attrId, ExtraAttrPtr attr);
    int getExtraAttrCount(const std::string& group) const;
    std::vector<std::string> getExtraAttrIDs(const std::string& group, const std::string& filterStr = "") const;
    bool attachExtraAttr(const std::string& group, const std::string& attrId);
    std::vector<std::string> getExtraAttrGroupNames() const;

    virtual std::map<std::string, std::string> getPolygonSetList() const;
    virtual PolygonSetPtr getPolygonSet(const std::string& id) const;
    short delPolygonSet(const std::string& id);

    virtual RecordPtr createRecord(const std::string& recordName) const;
    virtual std::string addRecord(const std::string& group, RecordPtr record);
    virtual bool deleteRecords(const std::string& group, const std::string& filterStr = "");
    virtual std::vector<RecordPtr> getRecords(const std::string& group, const std::string& filterStr = "") const;
    bool updateRecord(const std::string& recordId, RecordPtr record);
    int getRecordCount(const std::string& group) const;
    std::vector<std::string> getRecordIDs(const std::string& group, const std::string& filterStr = "") const;
    std::vector<std::string> getRecordGroupNames() const;
    virtual bool updateLayer(const std::string& layerId, PolygonSet& layer);

    /** ------------------operate polygonset-----------------------*/
    virtual PolygonSetPtr createPolygonSet(const std::string& name, double dbu = 0.001);
    virtual std::vector<std::string> addPolygonSets(const std::vector<PolygonSetPtr> layers);
    virtual std::vector<PolygonSetPtr> getPolygonSets() const;
    virtual bool deletePolygonSet(const std::string& id);
    virtual bool updatePolygonSet(const std::string& layerId, PolygonSetPtr& polygonset);
    // PolygonSet -> Layer
    // PatternSet -> PLIB
private:
    std::vector<std::string> getExtraAttrIDsByGroup_(const std::string& group) const;
    std::vector<std::string> getExtraAttrIDsByQuery_(const std::string& filterStr) const;
    bool detachExtraAttrs_(const std::string& group, const std::vector<std::string>& attrIds);
    bool updatePolygonSetToDB_(const std::string& id, const PolygonSet& layer);

    bool doAttachObjectFile(const std::string& name, const std::string& fileUri,
                            const std::map<std::string, std::string>& properties);
    void doCreateObjectFileAttr(Attribute& parentAttr, const std::string& name, const std::string& fileUri,
                                const std::map<std::string, std::string>& properties);
    std::string doGetObjectFileInfo(const std::string& name, std::map<std::string, std::string>* properties = nullptr);

public:
    short getCommonBBox(Int64& left, Int64& bottom, Int64& right, Int64& top, bool fetchFromLayer = true) const;
    std::vector<Int64> getCommonBBox();
    short addFlag(const Flag& flag, const std::string& flagset_name, bool update = false);
    short removeFlag(const std::string& flag_name, const std::string& flagset_name);
    std::vector<std::string> getFlagsetList() const;
    std::vector<Attribute*>* getFlagset(const std::string& flagset_name);
    short removeFlagset(const std::string& flagset_name);
    bool validateFlag(const Flag& flag) const;
    bool validateFlagsetName(std::string& flagset_name) const;
public:
    short addEdgeCondition(const EdgeCond& cond, const std::string& edgecondset_name, bool update = false);
    short removeEdgeCondition(const std::string& edgecond_name, const std::string& edgecondset_name);
    std::vector<std::string> getEdgeCondSetList();
    std::vector<Attribute*>* getEdgeCondSet(const std::string& edgecondset_name) const;
    short removeEdgeCondSet(const std::string& edgecondset_name);
    bool validateEdgeCond(const EdgeCond& edge_cond) const;
    bool validateEdgeCondSetName(std::string& edgecondset_name) const;
    short addEdgeInstru(const EdgeInstruction& instru, const std::string& edgeinstruset_name, bool update = false);
    short removeEdgeInstru(const std::string& edgeinstru_name, const std::string& edgeinstruset_name);
    std::vector<std::string> getEdgeInstruSetList();
    std::vector<Attribute*>* getEdgeInstruSet(const std::string& edgeinstruset_name) const;
    short removeEdgeInstruSet(const std::string& edgeinstruset_name);
    bool validateEdgeInstru(const EdgeInstruction& edge_instru) const;
    bool validateEdgeInstruSetName(std::string& edgeinstruset_name) const;
    short freezeEdge(int edge_id, int ring_id, int polygon_id, const std::string& layer_name,
                     const std::string& edgecondset_name);
    short thawFrozenEdge(int edge_id, int ring_id, int polygon_id, const std::string& layer_name,
                         const std::string& edgecondset_name);
    bool isEdgeFrozen(int edge_id, int ring_id, int polygon_id, const std::string& layer_name,
                      const std::string& edgecondset_name) const;
    std::vector<Attribute*>* getFrozenEdges(const std::string& edgecondset_name) const;


    short getPolygonSetsVec(std::vector<PolygonSetPtr>& polygonSet_vec) const;
    template <typename Archive>
    void serialize(Archive& ar);
    void adjustCoordsByDbu(PolygonSet& newPolygonset);
    void rotate(RotateOrientation orientation);
    void duplicate(Int64 width, Int64 height, Int64 px, Int64 py);
    short setDbu(double dbu);
    static std::set<std::string> getReadOnlyKeys();

private:
    std::string update();
    std::string addToPDB();
    short detachPolygonSet(const std::string& id);
    short attachImage(const Image& image);
    short detachImage(const std::string& id);
    std::map<std::string, std::string> getImageList();
    Image* getImage(const std::string& id);
    short delImage(const std::string& id);

private:
    std::string addToPDB_(int retry, std::string& err_msg);
    short attachPolygonSet_(const PolygonSet& polygon_set, int retry, std::string& err_msg);
    short detachPolygonSet_(const std::string& id, int retry, std::string& err_msg);
    bool validateE2BCond(const EdgeCond& edge_cond) const;
    bool validateFrozenEdge(const EdgeCond& frozen_edge) const;
    std::string addPolygonSet(const PolygonSet& polygon_set, int retry, std::string& err_msg);
    std::string addImage(const Image& image, int retry, std::string& err_msg);
    void updateDbuAndCoords(polygonSet_action_type action_type, varied_dbu_type dbu_type, double new_pattern_dbu);
    void updateLayerCountByLayers();
    short getDbuByLayers(const std::vector<PolygonSetPtr>& polygonSet_vec, double& new_pattern_dbu);
    void updatePolygonsetByDbu(std::vector<PolygonSetPtr>& polygonSet_vec, double new_pattern_dbu,
                               bool updateToDB = true);
    void updateCommonBBoxByPolygonset(const PolygonSet& polygonSet);
    void updateCommonBBoxByLayers(std::vector<PolygonSetPtr>& polygonSet_vec);
    void updateCommonBBoxByDbu(double old_pattern_dbu, double new_pattern_dbu);
    short getLayerCount();
    varied_dbu_type getVariedDbuType(double polygonSet_dbu);
    double getPatternDbuByDbus(double pattern_dbu, double polygonSet_dbu);
    bool isPolygonSetNeedRename(std::string& polygonSet_name);
    short getNewPolygonSetName(std::string& polygonSet_name);
    void updatePolygonSetName(PolygonSet& polygonSet);
    short setCommonBBox(Int64 left, Int64 bottom, Int64 right, Int64 top);
    void updateCreateInfo();
    void updateModifyInfo();
    std::string getUserName();
    std::string getCurrentTime();
    std::string getModifiedWith();
    short clonePolygonSet();
    std::string clonePattern();
    virtual bool isReadOnlyKey(const std::string& key) const;
    void setSourceType(pattern_source_type source_type);
    pattern_source_type getSourceType() const;
protected:
    PatternDB* m_pdb;
    std::set<std::string> polygonSet_name_list;
    static std::set<std::string> m_readonlyKeys;
    int MAX_RETRY_TIME;
    // mark pattern generated type.
    pattern_source_type m_source_type;
    friend class lib::pro::pdb2::detail::PatternDBImpl;
    friend class PatternSet;
};

typedef std::shared_ptr<Pattern> PatternPtr;

}  // namespace pdb2
}  // namespace pro
}  // namespace lib

#endif  // LIB_PRO_PDB2_PATTERN_H_
