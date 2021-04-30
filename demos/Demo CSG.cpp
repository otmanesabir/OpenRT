#include <fstream>
#include "openrt.h"
#include "core/timer.h"

using namespace rt;

void timeTests(const String& filePath, BoolOp operationType, int iterations, const Vec3f& cameraPos) {
    std::ofstream myFile;
    myFile.open (filePath);
    const Vec3f bgColor = RGB(1, 1, 1);
    const Size resolution = Size(1920, 1200);
    const float intensity = 5e4;

    // Shaders
    auto pShaderRed = std::make_shared<CShaderEyelight>(RGB(1, 0, 0));
    auto pShaderBlue = std::make_shared<CShaderEyelight>(RGB(0, 0, 1));

    // Light
    auto pLight = std::make_shared<CLightOmni>(intensity * RGB(1.0f, 0.839f, 0.494f), Vec3f(100, 150.0f, 100), false);

    for (int i = 6; i <= iterations + 6; i++) {
        // Scene
        CScene scene(bgColor);
        // Geometries
        auto 		solidSphere1 = CSolidSphere(pShaderRed, Vec3f(1, 0.1f, -13), 1.5f, i, false);
        auto 		solidSphere2 = CSolidSphere(pShaderBlue, Vec3f(0, 0.1f, -13), 1.5f, i, false);
        auto 	pComposize	 = std::make_shared<CCompositeGeometry>(solidSphere2, solidSphere1, operationType, 20, 3);

        // Camera
        auto targetCamera = std::make_shared<CCameraPerspectiveTarget>(resolution, cameraPos, pComposize->getBoundingBox().getCenter(), Vec3f(0, 1, 0), 45.0f);
        scene.add(targetCamera);
        scene.add(pLight);
        scene.add(pComposize);
        auto nPrimitives = solidSphere1.getPrims().size() +  solidSphere2.getPrims().size();
        scene.buildAccelStructure(20, 2);



        auto t1 = std::chrono::high_resolution_clock::now();
        Mat image = scene.render();
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();

        // log count to file
        myFile << nPrimitives << "," << duration << std::endl;
        std::cout << nPrimitives << "," << duration << std::endl;
        std::cout << "Progress: " << (float)(i-6)/(float)iterations * 100.0 << "%" << std::endl;

        std::string counterPath = "../../num.txt";
        int renderCount = 0;
        std::ifstream countFile(counterPath, std::ios::binary);
        countFile.read((char *) &renderCount, sizeof(renderCount));
        countFile.close();
        String initialPath;
        if (operationType == BoolOp::Union){
            initialPath = "../../timeTestsRenders/bin_union_";
        } else if (operationType == BoolOp::Intersection){
            initialPath = "../../timeTestsRenders/bin_intersection_";
        } else {
            initialPath = "../../timeTestsRenders/bin_difference_";
        }
        imwrite(initialPath + std::to_string(renderCount) + ".png", image);
        renderCount++;
        std::ofstream o(counterPath, std::ios::binary);
        o.write((char *) &renderCount, sizeof(renderCount));
        o.close();
    }
    myFile.close();
}

void nestingTests(std::string filePath, int steps) {
    std::ofstream myFile;
    myFile.open (filePath);
    const Vec3f bgColor = RGB(1, 1, 1);
    const Size resolution = Size(1920, 1200);
    const float intensity = 5e4;

    // Add everything to scene

    int zCounter = 0;
    int xCounter = 2;
    auto pShaderOrange = std::make_shared<CShaderEyelight>(RGB(254.0 / 255.0, 211.0 / 255.0, 71.0 / 255.0));
    auto solidSphere1 = CSolidSphere(pShaderOrange, Vec3f::all(0), 1.4, 24, false);
    auto solidSphere2 = CSolidSphere(pShaderOrange, Vec3f(1, 0, 0), 1.4, 24, false);
    ptr_prim_t composite = std::make_shared<CCompositeGeometry>(solidSphere1, solidSphere2, BoolOp::Union);

    for (int i = 2; i < steps; i++) {
        CScene scene(bgColor);

        if (i % 4 == 0) {
            zCounter++;
            xCounter = 0;
        }
        auto tempSphere = CSolidSphere(pShaderOrange, Vec3f(0 + xCounter++, 0, zCounter), 1.4, 24, false);
        composite = std::make_shared<CCompositeGeometry>(composite, tempSphere, BoolOp::Union);

        scene.add(composite);

        auto targetCamera = std::make_shared<CCameraPerspectiveTarget>(resolution, Vec3f(0 + (i / 4.0) , 5.0 + (i / 4.0), 3.0 + (i / 4.0)), composite->getBoundingBox().getCenter(), Vec3f(0, 1, 0), 45.0f);
        scene.add(targetCamera);

        // Light
        auto pLight = std::make_shared<CLightOmni>(intensity * RGB(1.0f, 0.839f, 0.494f), Vec3f(100, 150.0f, 100), false);

        scene.buildAccelStructure();

        auto t1 = std::chrono::high_resolution_clock::now();
        Mat image = scene.render();
        auto t2 = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();

        std::cout << solidSphere1.getPrims().size() * (i+1) << "," << (i-1) << "," << duration << std::endl;
        myFile << solidSphere1.getPrims().size() * (i+1) << "," << (i-1) << "," << duration << std::endl;

        std::string counterPath = "../../num.txt";
        int renderCount = 0;
        std::ifstream countFile(counterPath, std::ios::binary);
        countFile.read((char *) &renderCount, sizeof(renderCount));
        countFile.close();
        imwrite("../../nestingRenders/bin_nest_union_" + std::to_string(renderCount) + ".png", image);
        renderCount++;
        std::ofstream o(counterPath, std::ios::binary);

        o.write((char *) &renderCount, sizeof(renderCount));
        o.close();
    }
}


int sampleTest() {
    const Vec3f bgColor = RGB(1, 1, 1);
    const Size resolution = Size(1920, 1200);
    const float intensity = 5e4;

    // Scene
    CScene scene(bgColor);

    // Shaders
    auto pDarkBlue = std::make_shared<CShaderPhong>(scene, RGB(47 / 255.0, 60 / 255.0, 126 / 255.0), 0.1f, 0.9f, 0, 40.0f);
    auto pSunsetOrange = std::make_shared<CShaderPhong>(scene, RGB(251 / 255.0, 234 / 255.0, 235 / 255.0), 0.1f, 0.9f, 0, 40.0f);
    auto pShaderFloor = std::make_shared<CShaderPhong>(scene, RGB(1, 1, 1), 0.1f, 0.9f, 0.0f, 40.0f);

    auto pShaderOrange = std::make_shared<CShaderEyelight>(RGB(247.0 / 255.0, 127.0 / 255.0, 0.0 / 255.0));
    auto pShaderRed = std::make_shared<CShaderEyelight>(RGB(214.0/ 255.0, 40.0/255.0, 40/255.0));

    auto solidCylinder = CSolidCylinder(pShaderRed, Vec3f(1, 0, -13), 0.5, 4, 1, 24, true);
    auto solidCylinder2 = CSolidCylinder(pShaderRed, Vec3f(1, 0, -13), 0.5, 4, 1, 24, true);
    auto solidCylinder3 = CSolidCylinder(pShaderRed, Vec3f(1, 0, -13), 0.5, 4, 1, 24, true);

    // Getting the bounding box and setting the correct pivot point
    auto bBox = CBoundingBox();
    for (const auto& prim : solidCylinder2.getPrims()) {
        bBox.extend(prim->getBoundingBox());
    }
    solidCylinder2.setPivot(bBox.getCenter());
    solidCylinder2.transform(CTransform().rotate(Vec3f(0, 0, 1), 90).get());
    solidCylinder3.setPivot(bBox.getCenter());
    solidCylinder3.transform(CTransform().rotate(Vec3f(1, 0, 0), 90).get());

    // Left Tree Node
    //scene.add(solidCylinder);
    //scene.add(solidCylinder2);
    //scene.add(solidCylinder3);
    ptr_prim_t pCylinderComposite = std::make_shared<CCompositeGeometry>(solidCylinder, solidCylinder2, BoolOp::Union);
    ptr_prim_t pCylinderComposite2 = std::make_shared<CCompositeGeometry>(pCylinderComposite, solidCylinder3, BoolOp::Union);
    //scene.add(pCylinderComposite);
    //scene.add(pCylinderComposite2);

    auto solidBox = CSolidBox(pShaderOrange, Vec3f(Vec3f(1, 2, -13)), 4, 2, 2);
    auto solidSphere1 = CSolidSphere(pShaderRed, Vec3f(1, 2, -13), 1.3f, 30, true);
    ptr_prim_t pCompositeIntersection = std::make_shared<CCompositeGeometry>(solidSphere1, solidBox, BoolOp::Intersection);
    scene.add(solidBox);
    //scene.add(pCompositeIntersection);

    ptr_prim_t pRootNode = std::make_shared<CCompositeGeometry>(pCompositeIntersection,  solidCylinder, BoolOp::Difference);
    ptr_prim_t pRootNode1 = std::make_shared<CCompositeGeometry>(pRootNode,  solidCylinder2, BoolOp::Difference);
    ptr_prim_t pRootNode2 = std::make_shared<CCompositeGeometry>(pRootNode1,  solidCylinder3, BoolOp::Difference);
    //scene.add(pRootNode2);

    // cameras
    auto targetCamera = std::make_shared<CCameraPerspectiveTarget>(resolution, Vec3f(-4, 5, -3), solidBox.getPivot(), Vec3f(0, 1, 0), 45.0f);
    scene.add(targetCamera);

    scene.buildAccelStructure(20, 2);

    // Light
    auto pLight = std::make_shared<CLightOmni>(intensity * RGB(1.0f, 0.839f, 0.494f), Vec3f(100, 150.0f, 100), false);

    // Add everything to scene
    scene.add(pLight);
    Timer::start("Rendering... ");
    Mat img = scene.render(std::make_shared<CSamplerStratified>(2, true, true));
    Timer::stop();

    imshow("image", img);
    waitKey();
    return 0;
}

void viewPortTests(std::string filePath) {
    std::ofstream myFile;
    myFile.open (filePath);
    const Vec3f bgColor = RGB(1, 1, 1);
    const Size resolution = Size(1920, 1200);
    const float intensity = 5e4;

    auto pShaderOrange = std::make_shared<CShaderEyelight>(RGB(254.0 / 255.0, 211.0 / 255.0, 71.0 / 255.0));

    for (int i = 1; i < 21; i++) {

        CScene scene(bgColor);

        auto solidSphere1 = CSolidSphere(pShaderOrange, Vec3f::all(0), 1.5f*(i/5.0), 24, false);
        auto solidSphere2 = CSolidSphere(pShaderOrange, Vec3f(1.0f*(i/5.0), 0, 0), 1.5f*(i/5.0), 24, false);
        ptr_prim_t composite = std::make_shared<CCompositeGeometry>(solidSphere1, solidSphere2, BoolOp::Union);

        scene.add(composite);

        auto targetCamera = std::make_shared<CCameraPerspectiveTarget>(resolution, Vec3f(0, 5,15), composite->getBoundingBox().getCenter(), Vec3f(0, 1, 0), 45.0f);
        scene.add(targetCamera);

        // Light
        auto pLight = std::make_shared<CLightOmni>(intensity * RGB(1.0f, 0.839f, 0.494f), Vec3f(100, 150.0f, 100),
                                                   false);

        scene.buildAccelStructure();

        auto t1 = std::chrono::high_resolution_clock::now();
        Mat image = scene.render();
        auto t2 = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

        std::cout << solidSphere1.getPrims().size() * 2 << "," << i << "," << duration << std::endl;
        myFile << solidSphere1.getPrims().size() * 2 << "," << i << "," << duration << std::endl;

        std::string counterPath = "../../num.txt";
        int renderCount = 0;
        std::ifstream countFile(counterPath, std::ios::binary);
        countFile.read((char *) &renderCount, sizeof(renderCount));
        countFile.close();
        imwrite("../../viewPortRenders/base_port_union_" + std::to_string(renderCount) + ".png", image);
        renderCount++;
        std::ofstream o(counterPath, std::ios::binary);

        o.write((char *) &renderCount, sizeof(renderCount));
        o.close();
    }
}

int main() {
    //nestingTests("../../nesting_bin_union.txt");
    //viewPortTests("../../viewport_base_union.txt");

    // Low View Port : 31.656944444444445
    //timeTests("../../timeTests/bin_union_lvp.txt", BoolOp::Union, 300, Vec3f(4.5, 2, -15)); // already done
    //timeTests("../../timeTests/bin_intersection_lvp.txt", BoolOp::Intersection, 300, Vec3f(4.5, 2, -15));
    //timeTests("../../timeTests/bin_difference_lvp.txt", BoolOp::Difference, 300, Vec3f(4.5, 2, -15));

    // Mid View Port : 64.02730034722222
    //timeTests("../../timeTests/bin_union_mvp.txt", BoolOp::Union, 300, Vec3f(3, 2, -14));
    //timeTests("../../timeTests/bin_intersection_mvp.txt", BoolOp::Intersection, 300, Vec3f(3, 2, -14));
    //timeTests("../../timeTests/bin_difference_mvp.txt", BoolOp::Difference, 300, Vec3f(3, 2, -14));

    // High View Port : 98.26840277777778
    //timeTests("../../timeTests/bin_union_hvp.txt", BoolOp::Union, 300, Vec3f(2, 1, -14.5));
    //timeTests("../../timeTests/bin_intersection_hvp.txt", BoolOp::Intersection, 300, Vec3f(2, 1, -14.5));
    //timeTests("../../timeTests/bin_difference_hvp.txt", BoolOp::Difference, 300, Vec3f(2, 1, -14.5));
    nestingTests("../../timeTests/bin_nests_hvp.txt", 22);
    return 0;
}
