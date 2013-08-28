#pragma once

#include "../GLTransform.h"
#include "../IGLComponent.h"
#include <vector>
#include <map>

struct texCoord {
	texCoord(float u, float v = 0.0f) : u(u), v(v) { }
	float u, v;
};

struct material {
	material() {
		ka[0] = 0.2f; ka[1] = 0.2f; ka[2] = 0.2f;
		kd[0] = 0.8f; kd[1] = 0.8f; kd[2] = 0.8f;
		ks[0] = 1.0f; ks[1] = 1.0f; ks[2] = 1.0f;
		tr = 1.0f;
		ns = 0.0f;
		illum = 1;
	}
	float ka[3];
	float kd[3];
	float ks[3];
	float tr; // Aka d
	float ns;
	int illum;
	// TODO: Add maps
};

class GLMesh : public IGLComponent {
public:
	GLMesh(const int entityID); // Ctor that sets the entity ID.
	/**
	 * \brief Creates a new IGLComponent.
	 *
	 * \param[in] int entityID The entity this component belongs to.
	 * \returns IGLCompoent* the newly created component.
	 */
	void Initialize() ;

	/**
	 * \brief Returns the number of elements to draw for this component.
	 *
	 * Call this multiple times to get the elements for each mesh group. A return of 0 indicates all
	 * mesh groups have been returned, and the call loop should end.
	 * \param[in] const unsigned int group The mesh group to render.
	 * \returns unsigned int The number of elements to draw for the given mesh group.
	 */
	unsigned int MeshGroup_ElementCount(const unsigned int group = 0) const {
		if (group < (groupIndex.size() - 1)) {
			return (groupIndex[group+1] - groupIndex[group]) * 3;
		} else if (group > (groupIndex.size() - 1)) {
			return 0;
		} else {
			return (this->faces.size() - groupIndex[group]) * 3;
		}
	}

	void LoadMesh(std::string fname);

	void ParseMTL(std::string fname);

	/**
	 * \brief Add a vertex to the list.
	 * 
	 * \param[in] const Sigma::Vertex & v The vertex to add. It is copied.
	 */
	void AddVertex(const Sigma::Vertex& v) {
		this->verts2.push_back(v);
	}

	/**
	 * \brief Gets a vertex.
	 *
	 * Returns the vertex at the specific index.
	 * \param[in] const unsigned int index The index of the vertex to get.
	 * \returns   const Sigma::Vertex* The vertex at the index or nullptr if the index was invalid.
	 */
	const Sigma::Vertex* GetVertex(const unsigned int index) {
		try {
			return &this->verts2.at(index);
		} catch (std::out_of_range oor) {
			return nullptr;
		}
	}

	/**
	 * \brief Add a face to the list.
	 * 
	 * \param[in] const Sigma::Face & f The face to add. It is copied.
	 */
	void AddFace(const Sigma::Face& f) {
		this->faces2.push_back(f);
	}
		
	/**
	 * \brief Gets a face.
	 *
	 * Returns the face at the specific index.
	 * \param[in] const unsigned int index The index of the face to get.
	 * \returns   const Sigma::Face* The face at the index or nullptr if the index was invalid.
	 */
	const Sigma::Face* GetFace(const unsigned int index) {
		try {
			return &this->faces2.at(index);
		} catch (std::out_of_range oor) {
			return nullptr;
		}
	}
private:
	std::vector<unsigned int> groupIndex; // Stores which index in faces a group starts at.
	std::vector<Sigma::Face> faces; // Stores vectors of face groupings.
	std::vector<Sigma::Vertex> verts; // The verts that the faces refers to. Can be used for later refinement.
	std::vector<Sigma::Vertex> vertNorms; // The vertex normals for each vert.
	std::vector<Sigma::Face> faceNorms; // The index for each vert normal.
	std::vector<texCoord> texCoords; // The texture coords for each vertex.
	std::vector<Sigma::Face> texFaces; // The texture coords for each face.
	std::vector<color> colors;
	std::map<std::string, material> mats;

	std::vector<Sigma::Vertex> verts2;
	std::vector<Sigma::Face> faces2;
};
