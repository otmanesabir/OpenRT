#include "CompositeGeometry.h"

#include <utility>
#include <macroses.h>
#include "Ray.h"
#include "Transform.h"

namespace rt {

    // Constructor
    CCompositeGeometry::CCompositeGeometry(const CSolid &s1, const CSolid &s2, BoolOp operationType, int maxDepth,
                                           int maxPrimitives)
            : IPrim(nullptr), m_vPrims1(s1.getPrims()), m_vPrims2(s2.getPrims()), m_operationType(operationType)
#ifdef ENABLE_CSG_OPTIM
    , m_pBSPTree1(new CBSPTree()), m_pBSPTree2(new CBSPTree())
#endif
    {
        // Initializing the bounding box
        CBoundingBox boxA, boxB;
        for (const auto &prim : s1.getPrims())
            boxA.extend(prim->getBoundingBox());
        for (const auto &prim : s2.getPrims())
            boxB.extend(prim->getBoundingBox());
        Vec3f minPt = Vec3f::all(0);
        Vec3f maxPt = Vec3f::all(0);
        switch (m_operationType) {
            case BoolOp::Union:
                for (int i = 0; i < 3; i++) {
                    minPt[i] = MIN(boxA.getMinPoint()[i], boxB.getMinPoint()[i]);
                    maxPt[i] = MAX(boxA.getMaxPoint()[i], boxB.getMaxPoint()[i]);
                }
                break;
            case BoolOp::Intersection:
                for (int i = 0; i < 3; i++) {
                    minPt[i] = MAX(boxA.getMinPoint()[i], boxB.getMinPoint()[i]);
                    maxPt[i] = MIN(boxA.getMaxPoint()[i], boxB.getMaxPoint()[i]);
                }
                break;
            case BoolOp::Difference:
                for (int i = 0; i < 3; i++) {
                    minPt[i] = boxA.getMinPoint()[i];
                    maxPt[i] = boxA.getMaxPoint()[i];
                }
                break;
            default:
                break;
        }
        m_boundingBox = CBoundingBox(minPt, maxPt);
        m_origin = m_boundingBox.getCenter();
#ifdef ENABLE_CSG_OPTIM
        m_pBSPTree1->build(m_vPrims1, maxDepth, maxPrimitives);
        m_pBSPTree2->build(m_vPrims2, maxDepth, maxPrimitives);
#endif
    }

    bool CCompositeGeometry::intersect(Ray &ray) const {
        Ray modRay;
        modRay.dir = ray.dir;
        modRay.org = ray.org;
        modRay.counter = ray.counter;
        std::pair<Ray, Ray> range1(modRay, modRay);
        std::pair<Ray, Ray> range2(modRay, modRay);
        bool hasIntersection = false;
#ifdef ENABLE_CSG_OPTIM
        hasIntersection = m_pBSPTree1->intersect(range1.first);
        hasIntersection |= m_pBSPTree2->intersect(range2.first);
        if (m_operationType == BoolOp::Difference) {
            Ray r1 = modRay;
            Ray r2 = modRay;
            if (m_pBSPTree1->intersect_furthest(r1)) {
                range1.second = r1;
                hasIntersection = true;
            }
            if (m_pBSPTree2->intersect_furthest(r2)) {
                range2.second = r2;
                hasIntersection = true;
            }
        }
#else
        for (const auto &prim : m_vPrims1) {
            hasIntersection |= prim->intersect(range1.first);
            hasIntersection |= prim->intersect_furthest(range1.second);
        }
        for (const auto &prim : m_vPrims2) {
            hasIntersection |= prim->intersect(range2.first);
            hasIntersection |= prim->intersect_furthest(range2.second);
        }
#endif
        if (!hasIntersection)
            return false;
        Ray res;
        switch (m_operationType) {
            case BoolOp::Union:
                if (range1.first.hit && range2.first.hit) {
                    res = range1.first.t < range2.first.t ? range1.first : range2.first;
                    break;
                }
                if (range1.first.hit and !range2.first.hit) {
                    if (!range2.second.hit) {
                        res = range1.first;
                        break;
                    }
                    if (range2.second.t < range1.first.t) {
                        res = range2.second;
                        break;
                    }
                    res = range1.second;
                    break;
                } else if (!range1.first.hit and range2.first.hit) {
                    if (!range1.second.hit) {
                        res = range2.first;
                        break;
                    }
                    if (range1.second.t < range2.first.t) {
                        res = range1.second;
                        break;
                    }
                    res = range2.second;
                    break;
                }
                else if (!range1.first.hit && !range2.first.hit) {
                    if (!range1.second.hit && !range2.second.hit) {
                        return false;
                    } else {
                        res = range1.second.t > range2.second.t ? range1.second : range2.second;
                        break;
                    }
                }
                return false;
            case BoolOp::Intersection:
                if (range1.first.hit && range2.first.hit) {
                    if (range1.first.t < range2.first.t) {
                        if (range2.first.t < range1.second.t) {
                            res = range2.first;
                            break;
                        }
                    } else {
                        if (range1.first.t < range2.second.t) {
                            res = range1.first;
                            break;
                        }
                    }
                    return false;
                } else if (range1.first.hit && !range2.first.hit) {
                    if (!range2.second.hit) return false;
                    if (range1.first.t < range2.second.t && range2.second.t < range1.second.t) {
                        res = range1.first;
                        break;
                    }
                    return false;
                } else if (!range1.first.hit && range2.first.hit) {
                    if (!range1.second.hit) return false;
                    if (range2.first.t < range1.second.t && range1.second.t < range2.second.t) {
                        res = range2.first;
                        break;
                    }
                    return false;
                }
                if (!range1.first.hit && !range2.first.hit) {
                    if (!range1.second.hit || !range2.second.hit) {
                        return false;
                    }
                    res = range1.second.t > range2.second.t ? range1.second : range2.second;
                    break;
                }
                break;
            case BoolOp::Difference:
                if (!range1.first.hit && !range1.second.hit) {
                    return false;
                }
                if (!range2.first.hit && !range2.second.hit) {
                    if (range1.first.hit && range1.second.hit) {
                        res = range1.first;
                        break;
                    }
                    else {
                        return false;
                    }
                }
                if (range2.first.hit && !range2.second.hit && range1.first.hit && range1.second.hit) {
                    if (range2.first.t <= range1.first.t) {
                        res = range1.first;
                        break;
                    }
                }
                if (range1.second.hit && range2.second.hit) {
                    if (range1.first.hit) {
                        if (range2.first.hit) {
                            // case of all 4 intersections.
                            if (range1.first.t <= range2.first.t) {
                                if (range2.first.t < range1.second.t && range1.second.t < range2.second.t) {
                                    ray = range1.first;
                                    break;
                                } else if (range1.second.t < range2.first.t && range2.first.t < range2.second.t) {
                                    res = range1.first;
                                    break;
                                } else if (range2.first.t < range2.second.t && range2.second.t < range1.second.t) {
                                    res = range1.first;
                                    break;
                                } else {
                                    return false;
                                }
                            } else {
                                if (range1.first.t < range2.second.t && range2.second.t < range1.second.t) {
                                    res = range2.second;
                                    break;
                                } else if (range2.second.t < range1.first.t) {
                                    res = range1.first;
                                    break;
                                }
                            }
                        }
                    } else {
                        if (range2.first.hit) {
                            return false;
                        } else {
                            if (range2.second.t < range1.second.t) {
                                res = range2.second;
                                break;
                            }
                        }
                    }
                }
                return false;
            default:
                break;
        }
        if (res.hit) {
            if (ray.hit && ray.t < res.t) {
                return false;
            }
            ray = res;
            return true;
        }
        return false;
    }

    bool CCompositeGeometry::if_intersect(const Ray &ray) const {
        return intersect(lvalue_cast(Ray(ray))) || intersect_furthest(lvalue_cast(Ray(ray)));
    }

    void CCompositeGeometry::transform(const Mat &T) {
        CTransform tr;
        Mat T1 = tr.translate(-m_origin).get();
        Mat T2 = tr.translate(m_origin).get();

        // transform first geometry
        for (auto &pPrim : m_vPrims1) pPrim->transform(T * T1);
        for (auto &pPrim : m_vPrims1) pPrim->transform(T2);

        // transform second geometry
        for (auto &pPrim : m_vPrims2) pPrim->transform(T * T1);
        for (auto &pPrim : m_vPrims2) pPrim->transform(T2);

        // update pivots point
        for (int i = 0; i < 3; i++)
            m_origin.val[i] += T.at<float>(i, 3);
    }

    Vec3f CCompositeGeometry::getNormal(const Ray &ray) const {
        RT_ASSERT_MSG(false, "This method should never be called. Aborting...");
    }

    Vec2f CCompositeGeometry::getTextureCoords(const Ray &ray) const {
        RT_ASSERT_MSG(false, "This method should never be called. Aborting...");
    }

    bool CCompositeGeometry::intersect_furthest(Ray &ray) const {
        Ray modRay;
        modRay.dir = ray.dir;
        modRay.org = ray.org;
        modRay.counter = ray.counter;
        std::pair<Ray, Ray> range1(modRay, modRay);
        std::pair<Ray, Ray> range2(modRay, modRay);
        bool hasIntersection = false;
#ifdef ENABLE_CSG_OPTIM
        hasIntersection = m_pBSPTree1->intersect(range1.first);
        hasIntersection |= m_pBSPTree2->intersect(range2.first);
        if (m_operationType == BoolOp::Difference) {
            Ray r1 = modRay;
            Ray r2 = modRay;
            if (m_pBSPTree1->intersect_furthest(r1)) {
                range1.second = r1;
                hasIntersection = true;
            }
            if (m_pBSPTree2->intersect_furthest(r2)) {
                range2.second = r2;
                hasIntersection = true;
            }
        }
#else
        for (const auto &prim : m_vPrims1) {
            hasIntersection |= prim->intersect(range1.first);
            hasIntersection |= prim->intersect_furthest(range1.second);
        }
        for (const auto &prim : m_vPrims2) {
            hasIntersection |= prim->intersect(range2.first);
            hasIntersection |= prim->intersect_furthest(range2.second);
        }
#endif
        if (!hasIntersection)
            return false;
        Ray res;
        switch (m_operationType) {
            case BoolOp::Union:
                if (range1.first.hit && range2.first.hit) {
                    res = range1.first;
                    break;
                }
                if (range1.first.hit and !range2.first.hit) {
                    if (!range2.second.hit) {
                        res = range1.first;
                        break;
                    }
                    if (range2.second.t < range1.first.t) {
                        res = range2.second;
                        break;
                    }
                    res = range1.second;
                    break;
                } else if (!range1.first.hit and range2.first.hit) {
                    if (!range1.second.hit) {
                        res = range2.first;
                        break;
                    }
                    if (range1.second.t < range2.first.t) {
                        res = range1.second;
                        break;
                    }
                    res = range2.second;
                    break;
                }
                else if (!range1.first.hit && !range2.first.hit) {
                    if (!range1.second.hit && !range2.second.hit) {
                        return false;
                    } else {
                        res = range1.second.t > range2.second.t ? range1.second : range2.second;
                        break;
                    }
                }
                return false;
            case BoolOp::Intersection:
                if (range1.first.hit && range2.first.hit) {
                    if (range1.first.t < range2.first.t) {
                        if (range2.first.t < range1.second.t) {
                            res = range2.first;
                            break;
                        }
                    } else {
                        if (range1.first.t < range2.second.t) {
                            res = range1.first;
                            break;
                        }
                    }
                    return false;
                } else if (range1.first.hit && !range2.first.hit) {
                    if (!range2.second.hit) return false;
                    if (range1.first.t < range2.second.t && range2.second.t < range1.second.t) {
                        res = range1.first;
                        break;
                    }
                    return false;
                } else if (!range1.first.hit && range2.first.hit) {
                    if (!range1.second.hit) return false;
                    if (range2.first.t < range1.second.t && range1.second.t < range2.second.t) {
                        res = range2.first;
                        break;
                    }
                    return false;
                }
                if (!range1.first.hit && !range2.first.hit) {
                    if (!range1.second.hit || !range2.second.hit) {
                        return false;
                    }
                    res = range1.second.t > range2.second.t ? range1.second : range2.second;
                    break;
                }
                break;
            case BoolOp::Difference:
                if (!range1.first.hit && !range1.second.hit) {
                    return false;
                }
                if (!range2.first.hit && !range2.second.hit) {
                    if (range1.second.hit) {
                        res = range1.second;
                        break;
                    } else {
                        return false;
                    }
                }
                if (range2.first.hit && !range2.second.hit && range1.first.hit && range1.second.hit) {
                    if (range2.first.t <= range1.first.t) {
                        res = range1.second;
                        break;
                    }
                }
                if (range1.second.hit && range2.second.hit) {
                    if (range1.first.hit) {
                        if (range2.first.hit) {
                            // case of all 4 intersections.
                            if (range1.first.t <= range2.first.t) {
                                if (range2.first.t < range1.second.t && range1.second.t < range2.second.t) {
                                    ray = range2.first;
                                    break;
                                } else if (range1.second.t < range2.first.t && range2.first.t < range2.second.t) {
                                    res = range1.second;
                                    break;
                                } else if (range2.first.t < range2.second.t && range2.second.t < range1.second.t) {
                                    res = range1.second;
                                    break;
                                } else {
                                    return false;
                                }
                            } else {
                                if (range1.first.t < range2.second.t  && range2.second.t < range1.second.t) {
                                    res = range1.second;
                                    break;
                                } else if (range2.second.t < range1.first.t && range1.first.t < range1.second.t) {
                                    res = range1.second;
                                    break;
                                }
                                return false;
                            }
                        } else {
                            res = range1.second;
                            break;
                        }
                    } else {
                        if (range2.first.hit) {
                            if (range2.first.t < range1.second.t) {
                                res = range2.first;
                                break;
                            }
                        } else {
                            if (range2.second.t < range1.second.t) {
                                res = range1.second;
                                break;
                            }
                        }
                    }
                }
                return false;
            default:
                break;
        }
        if (res.hit) {
            if (ray.hit && ray.t < res.t) {
                return false;
            }
            ray = res;
            return true;
        }
        return false;
    }
}
