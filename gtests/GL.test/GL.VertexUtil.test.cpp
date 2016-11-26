#include <Saba/GL/GLVertexUtil.h>

#include <gtest/gtest.h>

TEST(GLTest, VertexUtilTest)
{
	saba::MeshBuilder mb;

	EXPECT_EQ(0, mb.GetPolygonCount());

	std::vector<glm::vec3>* positions;
	std::vector<glm::vec3>* normals;
	std::vector<glm::vec2>* uvs;
	auto posID = mb.AddComponent(&positions);
	auto norID = mb.AddComponent(&normals);
	auto uvID = mb.AddComponent(&uvs);
	EXPECT_EQ(0, posID);
	EXPECT_EQ(1, norID);
	EXPECT_EQ(2, uvID);
	EXPECT_NE(nullptr, positions);
	EXPECT_NE(nullptr, normals);
	EXPECT_NE(nullptr, uvs);

	positions->push_back(glm::vec3(0, 1, 0));
	positions->push_back(glm::vec3(-1, -1, 0));
	positions->push_back(glm::vec3(-1, -1, 0));
	normals->push_back(glm::vec3(0, 0, 1));
	normals->push_back(glm::vec3(0, 0, -1));
	uvs->push_back(glm::vec2(0.5f, 1.0f));
	uvs->push_back(glm::vec2(0.0f, 0.0f));
	uvs->push_back(glm::vec2(1.0f, 0.0f));

	auto poly1ID = mb.AddPolygon();
	auto poly2ID = mb.AddPolygon();
	EXPECT_EQ(0, poly1ID);
	EXPECT_EQ(1, poly2ID);

	mb.SetPolygon(poly1ID, posID, saba::MeshBuilder::Polygon(0, 1, 2));
	mb.SetPolygon(poly2ID, posID, 2, 1, 0);

	mb.SetPolygon(poly1ID, norID, 0, 0, 0);
	mb.SetPolygon(poly2ID, norID, 1, 1, 1);

	mb.SetPolygon(poly1ID, uvID, 0, 1, 2);
	mb.SetPolygon(poly2ID, uvID, 2, 1, 0);

	mb.SetFaceMaterial(poly2ID, 10);

	mb.SortFaces();

	auto posVBO = mb.CreateVBO(posID);
	auto norVBO = mb.CreateVBO(norID);
	auto uvVBO = mb.CreateVBO(uvID);

	EXPECT_NE(0, posVBO.Get());
	EXPECT_NE(0, norVBO.Get());
	EXPECT_NE(0, uvVBO.Get());

	auto posBinder = mb.MakeVertexBinder(posID);
	auto norBinder = mb.MakeVertexBinder(norID);
	auto uvBinder = mb.MakeVertexBinder(uvID);

	EXPECT_EQ(3, posBinder.m_elementNum);
	EXPECT_EQ(GL_FLOAT, posBinder.m_elementType);
	EXPECT_EQ(sizeof(float) * 3, posBinder.m_stride);
	EXPECT_EQ(0, posBinder.m_offset);

	EXPECT_EQ(3, norBinder.m_elementNum);
	EXPECT_EQ(GL_FLOAT, norBinder.m_elementType);
	EXPECT_EQ(sizeof(float) * 3, norBinder.m_stride);
	EXPECT_EQ(0, norBinder.m_offset);

	EXPECT_EQ(2, uvBinder.m_elementNum);
	EXPECT_EQ(GL_FLOAT, uvBinder.m_elementType);
	EXPECT_EQ(sizeof(float) * 2, uvBinder.m_stride);
	EXPECT_EQ(0, uvBinder.m_offset);

	std::vector<saba::MeshBuilder::SubMesh> subMeshes;
	mb.MakeSubMeshList(&subMeshes);

	EXPECT_EQ(0, subMeshes[0].m_startIndex);
	EXPECT_EQ(3, subMeshes[0].m_numVertices);
	EXPECT_EQ(-1, subMeshes[0].m_material);

	EXPECT_EQ(3, subMeshes[1].m_startIndex);
	EXPECT_EQ(3, subMeshes[1].m_numVertices);
	EXPECT_EQ(10, subMeshes[1].m_material);

	mb.Clear();
	EXPECT_EQ(0, mb.GetPolygonCount());
	EXPECT_EQ(0, mb.GetComponentCount());
}
