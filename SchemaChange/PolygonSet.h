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

#ifndef LIB_PRO_PDB2_POLYGONSET_H_
#define LIB_PRO_PDB2_POLYGONSET_H_
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "lib/pro/pdb2/attribute.h"
namespace lib {
namespace infra {
namespace geometry {
class IPolygon;
class Polygon;
}  // namespace geometry
}  // namespace infra

namespace pro {
namespace pdb2 {

enum class RotateOrientation : char {
    R0,
    R90,
    R180,
    R270,
    MX,
    MY,
    MXR90,
    MYR90,
};

enum class DuplicationResult : char { DupFailed, DupNoDirection, DupInXDirection, DupInYDirection, DupInXYDirection };

class PolygonSet : public IAttribute {
public:
    explicit PolygonSet(const std::string& name, const double& dbu = 0.001);
    explicit PolygonSet(const IAttribute& attr);
    virtual ~PolygonSet();
    static PolygonSet* createPolygonSet(const std::string& name, const double& dbu = 0.001);
    short addPolygon(Polygon& polygon);
    Polygon* getPolygon(const std::string& name) const;
    short removePolygon(const std::string& name);
    unsigned int getNumberOfPolygons() const;
    short setBBox(Int64 left, Int64 bottom, Int64 right, Int64 top);
    std::vector<Int64> getBBox();
    std::vector<double> getSourceBBox();
    short setSourceBBox(double left, double bottom, double right, double top);
#ifndef SWIG
    short getBBox(Int64& left, Int64& bottom, Int64& right, Int64& top) const;
    short getMinBBox(Int64& left, Int64& bottom, Int64& right, Int64& top) const;
    void updateBBoxByDbu(double old_dbu);
    void updateBBoxBySize(Int64 width, Int64 height);
    void updateCoordsByDbu();
    void clearPolygons();
    void rotate(const _POINT_& centerPoint, RotateOrientation orientation);
    DuplicationResult duplicate(Int64 width, Int64 height, Int64 px, Int64 py);
    bool bias(double value);
    void scale(double value);
    void removeLinePolygon();
    bool hasOverlap() const;
    std::vector<_POINT_> calculateOffsets(Int64 width, Int64 height, Int64 px, Int64 py, DuplicationResult& ret) const;
    bool isValidDuplicationParameters(Int64 width, Int64 height, Int64 px, Int64 py) const;
    std::vector<infra::geometry::Polygon*> toInfraPolygons(bool snapped = false) const;
    template <typename Archive>
    void serialize(Archive& ar);
    void setDbu(double dbu);
    void tone();
    static std::set<std::string> getReadOnlyKeys();
#endif
private:
    virtual bool isReadOnlyKey(const std::string& key) const;
    std::string getAvailablePolygonName() const;
    static std::set<std::string> m_readonlyKeys;
    friend class Pattern;
};

typedef boost::shared_ptr<PolygonSet> PolygonSetPtr;
typedef boost::shared_ptr<Polygon> PolygonPtr;

}  // namespace pdb2
}  // namespace pro
}  // namespace lib

#endif  // LIB_PRO_PDB2_POLYGONSET_H_
