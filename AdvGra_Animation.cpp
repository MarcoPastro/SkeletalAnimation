#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <string>
#include <vector>

aiNode* FindBoneNode(const std::string& boneName, const aiNode* rootNode) {
    if (rootNode) {
        if (std::string(rootNode->mName.C_Str()) == boneName) {
            return const_cast<aiNode*>(rootNode);
        }

        for (unsigned int i = 0; i < rootNode->mNumChildren; i++) {
            aiNode* foundNode = FindBoneNode(boneName, rootNode->mChildren[i]);
            if (foundNode) {
                return foundNode;
            }
        }
    }

    return nullptr;
}

aiMatrix4x4 CalculateCumulativeBoneTransformation(const std::string& boneName, const aiNode* rootNode) {
    aiMatrix4x4 cumulativeTransformation;
    aiNode* boneNode = FindBoneNode(boneName, rootNode);

    while (boneNode) {
        cumulativeTransformation = boneNode->mTransformation * cumulativeTransformation;
        boneNode = boneNode->mParent;
    }

    return cumulativeTransformation;
}

void TransformVerticesInfluencedByBone(const aiMesh* mesh, const aiMatrix4x4& transformation,
    const aiMatrix4x4& cumulativeBoneTransformation, const std::string& boneName) {
    for (unsigned int j = 0; j < mesh->mNumBones; j++) {
        const aiBone* bone = mesh->mBones[j];
        if (std::string(bone->mName.C_Str()) == boneName) {
            for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
                aiVector3D originalPosition = mesh->mVertices[bone->mWeights[weightIndex].mVertexId];
                aiVector3D transformedPosition = transformation * originalPosition;//cumulativeBoneTransformation *
                mesh->mVertices[bone->mWeights[weightIndex].mVertexId] = transformedPosition;
            }

        }
    }
}
std::string AiStringToStdString(const aiString& aiStr) {
    return std::string(aiStr.C_Str());
}
int main()
{

    Assimp::Importer importer1;
    Assimp::Importer importer2;

    unsigned int importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_LimitBoneWeights | aiProcess_JoinIdenticalVertices;
    const aiScene* scene = importer1.ReadFile("Mesh/Praying.fbx", importFlags);

    if (nullptr == scene) {
        std::cout << "Error" << std::endl;
    }

    importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_LimitBoneWeights | aiProcess_JoinIdenticalVertices;
    const aiScene* animatedScene = importer2.ReadFile("Mesh/Praying.fbx", importFlags);

    if (nullptr == animatedScene) {
        std::cout << "Error" << std::endl;
    }

    unsigned int animationIndex = 0;
    unsigned int boneIndex = 6;
    double keyframeTime = 5.0;

    aiAnimation* animation = animatedScene->mAnimations[animationIndex];
    aiNodeAnim* boneAnimation = animation->mChannels[boneIndex];
    aiMatrix4x4 boneTransformation;
    for (unsigned int keyframeIndex = 0; keyframeIndex < boneAnimation->mNumPositionKeys; ++keyframeIndex) {
        if (boneAnimation->mPositionKeys[keyframeIndex].mTime >= keyframeTime) {
            aiMatrix4x4 translationMatrix;
            aiMatrix4x4 rotationMatrix;
            aiMatrix4x4 scaleMatrix;

            aiMatrix4x4::Translation(boneAnimation->mPositionKeys[keyframeIndex].mValue, translationMatrix);
            aiQuaternion rotation = boneAnimation->mRotationKeys[keyframeIndex].mValue;
            rotationMatrix.FromEulerAnglesXYZ(rotation.x, rotation.y, rotation.z);
            aiMatrix4x4::Scaling(boneAnimation->mScalingKeys[keyframeIndex].mValue, scaleMatrix);

            boneTransformation = translationMatrix * rotationMatrix * scaleMatrix;
            break;
        }
    }
    aiMesh* mesh = scene->mMeshes[0];
    aiBone* targetBone = mesh->mBones[boneIndex];

    std::string boneToTransform = AiStringToStdString(targetBone->mName);

    aiMatrix4x4 transformation = boneTransformation;

    aiMatrix4x4 cumulativeBoneTransformation = CalculateCumulativeBoneTransformation(boneToTransform, scene->mRootNode);

    TransformVerticesInfluencedByBone(mesh, transformation, cumulativeBoneTransformation, boneToTransform);

    Assimp::Exporter exporter;

    exporter.Export(scene, "obj", "Mesh/output.obj");

    importer1.FreeScene();
    importer2.FreeScene();
}
