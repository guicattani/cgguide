#ifndef CLASS_HEADER_MODEL_LOADER
#define CLASS_HEADER_MODEL_LOADER
#include "model_loader.h"
#endif

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#endif
using namespace std;
using namespace glm;

// Função que carrega uma imagem para ser utilizada como textura
GLuint LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    stbi_set_flip_vertically_on_load(true);
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
         printf("OK.\n");
    }
    else
    {
        cout << "Texture failed to load at path: " << filename << endl;
        stbi_image_free(data);
    }

    g_NumLoadedTextures += 1;
    return textureID;
}

void RenderText(Shader &s, GLuint VAO, GLuint VBO, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state
    s.setVec3("textColor", vec3(color.x, color.y, color.z));
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void AddModelToVirtualScene(ObjModel *model, map<string, SceneObject> &virtualScene)
{
  GLuint vertex_array_object_id;
  glGenVertexArrays(1, &vertex_array_object_id);
  glBindVertexArray(vertex_array_object_id);

  std::vector<GLuint> indices;
  std::vector<float> model_coefficients;
  std::vector<float> normal_coefficients;
  std::vector<float> texture_coefficients;

  for (size_t shape = 0; shape < model->shapes.size(); ++shape)
  {
    size_t first_index = indices.size();
    size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

    const float minval = std::numeric_limits<float>::min();
    const float maxval = std::numeric_limits<float>::max();

    vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
    vec3 bbox_max = glm::vec3(minval,minval,minval);

    for (size_t triangle = 0; triangle < num_triangles; ++triangle)
    {
      assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

      for (size_t vertex = 0; vertex < 3; ++vertex)
      {
        tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];

        indices.push_back(first_index + 3 * triangle + vertex);

        const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
        const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
        const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];
        //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
        model_coefficients.push_back(vx);   // X
        model_coefficients.push_back(vy);   // Y
        model_coefficients.push_back(vz);   // Z
        model_coefficients.push_back(1.0f); // W

        bbox_min.x = std::min(bbox_min.x, vx);
        bbox_min.y = std::min(bbox_min.y, vy);
        bbox_min.z = std::min(bbox_min.z, vz);
        bbox_max.x = std::max(bbox_max.x, vx);
        bbox_max.y = std::max(bbox_max.y, vy);
        bbox_max.z = std::max(bbox_max.z, vz);

        // Inspecionando o código da tinyobjloader, o aluno Bernardo
        // Sulzbach (2017/1) apontou que a maneira correta de testar se
        // existem normais e coordenadas de textura no ObjModel é
        // comparando se o índice retornado é -1. Fazemos isso abaixo.

        if (idx.normal_index != -1)
        {
          const float nx = model->attrib.normals[3 * idx.normal_index + 0];
          const float ny = model->attrib.normals[3 * idx.normal_index + 1];
          const float nz = model->attrib.normals[3 * idx.normal_index + 2];
          normal_coefficients.push_back(nx);   // X
          normal_coefficients.push_back(ny);   // Y
          normal_coefficients.push_back(nz);   // Z
          normal_coefficients.push_back(0.0f); // W
        }

        if (idx.texcoord_index != -1)
        {
          const float u = model->attrib.texcoords[2 * idx.texcoord_index + 0];
          const float v = model->attrib.texcoords[2 * idx.texcoord_index + 1];
          texture_coefficients.push_back(u);
          texture_coefficients.push_back(v);
        }
      }
    }

    size_t last_index = indices.size() - 1;

    vec3 bbox_center = glm::vec3((bbox_min.x + bbox_max.x) / 2,
                                 (bbox_min.y + bbox_max.y) / 2,
                                 (bbox_min.z + bbox_max.z) / 2);

    SceneObject theobject;
    theobject.name = model->shapes[shape].name;
    theobject.first_index = (void *)first_index;          // Primeiro índice
    theobject.num_indices = last_index - first_index + 1; // Número de indices
    theobject.rendering_mode = GL_TRIANGLES;              // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
    theobject.bbox_min = bbox_min;
    theobject.bbox_center = bbox_center;
    theobject.bbox_max = bbox_max;
    theobject.vertex_array_object_id = vertex_array_object_id;

    virtualScene[model->shapes[shape].name] = theobject;
  }

  GLuint VBO_model_coefficients_id;
  glGenBuffers(1, &VBO_model_coefficients_id);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
  glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
  GLuint location = 0;            // "(location = 0)" em "shader_vertex.glsl"
  GLint number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
  glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(location);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  if (!normal_coefficients.empty())
  {
    GLuint VBO_normal_coefficients_id;
    glGenBuffers(1, &VBO_normal_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
    location = 1;             // "(location = 1)" em "shader_vertex.glsl"
    number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  if (!texture_coefficients.empty())
  {
    GLuint VBO_texture_coefficients_id;
    glGenBuffers(1, &VBO_texture_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
    location = 2;             // "(location = 1)" em "shader_vertex.glsl"
    number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  GLuint indices_id;
  glGenBuffers(1, &indices_id);

  // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
  //

  // "Desligamos" o VAO, evitando assim que operações posteriores venham a
  // alterar o mesmo. Isso evita bugs.
  glBindVertexArray(0);
}

// Função que desenha um objeto armazenado em virtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(SceneObject sceneObject)
{
  // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
  // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
  // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
  glBindVertexArray(sceneObject.vertex_array_object_id);

  // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
  // apontados pelo VAO como linhas. Veja a definição de
  // virtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
  // a documentação da função glDrawElements() em
  // http://docs.gl/gl3/glDrawElements.
  glDrawElements(
      sceneObject.rendering_mode,
      sceneObject.num_indices,
      GL_UNSIGNED_INT,
      (void *)sceneObject.first_index);

  // "Desligamos" o VAO, evitando assim que operações posteriores venham a
  // alterar o mesmo. Isso evita bugs.
  glBindVertexArray(0);
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel *model)
{
  if (!model->attrib.normals.empty())
    return;

  // Primeiro computamos as normais para todos os TRIÂNGULOS.
  // Segundo, computamos as normais dos VÉRTICES através do método proposto
  // por Gourad, onde a normal de cada vértice vai ser a média das normais de
  // todas as faces que compartilham este vértice.

  size_t num_vertices = model->attrib.vertices.size() / 3;

  vector<int> num_triangles_per_vertex(num_vertices, 0);
  vector<vec4> vertex_normals(num_vertices, vec4(0.0f, 0.0f, 0.0f, 0.0f));

  for (size_t shape = 0; shape < model->shapes.size(); ++shape)
  {
    size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

    for (size_t triangle = 0; triangle < num_triangles; ++triangle)
    {
      assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

      vec4 vertices[3];
      for (size_t vertex = 0; vertex < 3; ++vertex)
      {
        tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
        const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
        const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
        const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];
        vertices[vertex] = vec4(vx, vy, vz, 1.0);
      }

      const vec4 a = vertices[0];
      const vec4 b = vertices[1];
      const vec4 c = vertices[2];

      const vec4 n = crossproduct(b - a, c - a);

      for (size_t vertex = 0; vertex < 3; ++vertex)
      {
        tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
        num_triangles_per_vertex[idx.vertex_index] += 1;
        vertex_normals[idx.vertex_index] += n;
        model->shapes[shape].mesh.indices[3 * triangle + vertex].normal_index = idx.vertex_index;
      }
    }
  }

  model->attrib.normals.resize(3 * num_vertices);

  for (size_t i = 0; i < vertex_normals.size(); ++i)
  {
    vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
    n /= norm(n);
    model->attrib.normals[3 * i + 0] = n.x;
    model->attrib.normals[3 * i + 1] = n.y;
    model->attrib.normals[3 * i + 2] = n.z;
  }
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel *model)
{
  const tinyobj::attrib_t &attrib = model->attrib;
  const vector<tinyobj::shape_t> &shapes = model->shapes;
  const vector<tinyobj::material_t> &materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++)
  {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++)
  {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++)
  {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++)
  {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++)
    {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++)
      {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++)
    {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j)
      {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1))
        {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j)
      {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1))
        {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j)
      {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1))
        {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++)
  {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    map<string, string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    map<string, string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++)
    {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}
