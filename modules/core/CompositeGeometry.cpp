#include "CompositeGeometry.h"

#include <utility>
#include <macroses.h>
#include "Ray.h"
#include "Transform.h"

namespace rt {
	
	// Constructor
	CCompositeGeometry::CCompositeGeometry(const CSolid &s1, const CSolid &s2, BoolOp operationType)
		: IPrim(nullptr)
		, m_vpPrims1(s1.getPrims())
		, m_vpPrims2(s2.getPrims())
		, m_operationType(operationType)
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
				RT_WARNING("This functionality is not implemented yet");
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
	}

	bool CCompositeGeometry::intersect(Ray &ray) const {

		std::pair<Ray, Ray> range1(ray, ray);
		std::pair<Ray, Ray> range2(ray, ray);
		range1.second.t = -Infty;
		range2.second.t = -Infty;
		bool hasIntersection = false;
		for (auto &prim : m_vpPrims1) {
			Ray r = ray;
            if (prim->intersect(r)) {
				if (r.t < range1.first.t)
					range1.first = r;
				if (r.t > range1.second.t)
					range2.second = r;
				hasIntersection = true;
			}
		}
		for (auto &prim : m_vpPrims2) {
			Ray r = ray;
			if (prim->intersect(r)) {
				if (r.t < range2.first.t)
					range2.first = r;
				if (r.t > range2.second.t)
					range2.second = r;
				hasIntersection = true;
			}
		}
		if (!hasIntersection)
			return false;
		double t = 0;
		switch (m_operationType) {
			case BoolOp::Union:
				t = MIN(range1.first.t, range2.first.t);
				if (abs(t - range1.first.t) < Epsilon)
					ray = range1.first;
				else if (abs(t - range2.first.t) < Epsilon)
					ray = range2.first;
				break;
			case BoolOp::Intersection:
				t = MAX(range1.first.t, range2.first.t);
				if (abs(t) >= Infty)
					return false;
				if (abs(t - range1.first.t) < Epsilon)
					ray = range1.first;
				else if (abs(t - range2.first.t) < Epsilon)
					ray = range2.first;
				break;
			case BoolOp::Difference:
				RT_WARNING("This functionality is not implemented yet");
				break;
			default:
				break;
		}
		return true;
	}

	bool CCompositeGeometry::if_intersect(const Ray &ray) const {
		return intersect(lvalue_cast(Ray(ray)));
	}

	void CCompositeGeometry::transform(const Mat &T) {
        CTransform tr;
        Mat T1 = tr.translate(-m_origin).get();
        Mat T2 = tr.translate(m_origin).get();

        // Apply transformation on first solid
        for (auto& pPrim : m_vpPrims1) pPrim->transform(T * T1);
        for (auto& pPrim : m_vpPrims1) pPrim->transform(T2);

        // Apply transformation on second solid
        for (auto& pPrim : m_vpPrims2) pPrim->transform(T * T1);
        for (auto& pPrim : m_vpPrims2) pPrim->transform(T2);

        // Update origin point
        for (int i = 0; i < 3; i++)
            m_origin.val[i] += T.at<float>(i, 3);
	}

	Vec3f CCompositeGeometry::getNormal(const Ray &ray) const {
		RT_ASSERT_MSG(false, "This method should never be called. Aborting...");
	}

	Vec2f CCompositeGeometry::getTextureCoords(const Ray &ray) const {
        RT_ASSERT_MSG(false, "This method should never be called. Aborting...");
	}
}
