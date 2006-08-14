
#include "scene.h"
#include "effect.h"

// Include GLEW before anything else.
#include <GL/glew.h>

#include <QtCore/QString>
#include <QtCore/QFile>
#include <QtGui/QFileDialog>

#include <math.h>


extern void drawTeapot();

class DisplayListScene : public Scene
{
public:
	DisplayListScene() : m_dlist(0)
	{
	}

	virtual ~DisplayListScene()
	{
		if( m_dlist != 0 ) {
			glDeleteLists(m_dlist, 1);
		}
	}
	

	virtual void draw(Effect* effect) const
	{
		Q_UNUSED(effect);
		
		// Reset default material.
		GLfloat ka[] = {0.1f, 0.0f, 0.0f, 0.0f};
		GLfloat kd[] = {0.9f, 0.0f, 0.0f, 0.0f};
		GLfloat ks[] = {1.0f, 1.0f, 1.0f, 0.0f};
		glMaterialfv(GL_FRONT, GL_AMBIENT, ka);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, kd);
		glMaterialfv(GL_FRONT, GL_SPECULAR, ks);
		glMaterialf(GL_FRONT, GL_SHININESS, 8);
		
		glCallList(m_dlist);
	}
	
	virtual void transform() const
	{
	}
	
	virtual void setupMenu(QMenu * menu) const
	{
		Q_UNUSED(menu);
	}

protected:
	GLuint m_dlist;
};


class TeapotScene : public DisplayListScene
{
public:
	TeapotScene()
	{
		m_dlist = glGenLists(1);
		glNewList(m_dlist, GL_COMPILE);
		drawTeapot();
		glEndList();
	}
	
	virtual void transform() const 
	{
		// Object transform:
		glRotated(270.0, 1.0, 0.0, 0.0);
		glScaled(0.5, 0.5, 0.5);
		glTranslated(0.0, 0.0, -1.5);			
	}
};

// Teapot scene factory.
class TeapotSceneFactory : public SceneFactory
{
public:
	virtual QString name() const
	{
		return tr("Teapot");
	}
	virtual QString description() const
	{
		return tr("Teapot");
	}
	virtual QIcon icon() const
	{
		return QIcon();
	}
	virtual Scene * createScene() const
	{
		return new TeapotScene();
	}
};

REGISTER_SCENE_FACTORY(TeapotSceneFactory);



class QuadScene : public DisplayListScene
{
public:
	QuadScene()
	{
		m_dlist = glGenLists(1);
		glNewList(m_dlist, GL_COMPILE);
		glBegin(GL_QUADS);
			glNormal3f(0, 0, 1);
			glTexCoord2f(0, 1);
			glVertex2f(-1, -1);

			glNormal3f(0, 0, 1);
			glTexCoord2f(1, 1);
			glVertex2f(1, -1);

			glNormal3f(0, 0, 1);
			glTexCoord2f(1, 0);
			glVertex2f(1, 1);

			glNormal3f(0, 0, 1);
			glTexCoord2f(0, 0);
			glVertex2f(-1, 1);
		glEnd();
		glEndList();
	}
};

// Quad scene factory.
class QuadSceneFactory : public SceneFactory
{
public:
	virtual QString name() const
	{
		return tr("Quad");
	}
	virtual QString description() const
	{
		return tr("Quad");
	}
	virtual QIcon icon() const
	{
		return QIcon();
	}
	virtual Scene * createScene() const
	{
		return new QuadScene();
	}
};

REGISTER_SCENE_FACTORY(QuadSceneFactory);


class CubeScene : public DisplayListScene
{
public:
	CubeScene()
	{
		m_dlist = glGenLists(1);
		glNewList(m_dlist, GL_COMPILE);
		glBegin(GL_QUADS);
		
		// Front Face
		glNormal3f(0, 0, 1);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
		// Back Face
		glNormal3f(0, 0, -1);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
		// Top Face
		glNormal3f(0, 1, 0);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		// Bottom Face
		glNormal3f(0, -1, 0);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
		// Right face
		glNormal3f(1, 0, 0);
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
		glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
		// Left Face
		glNormal3f(-1, 0, 0);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
		
		glEnd();
		glEndList();
	}
};

// Cube scene factory.
class CubeSceneFactory : public SceneFactory
{
public:
	virtual QString name() const
	{
		return tr("Cube");
	}
	virtual QString description() const
	{
		return tr("Cube");
	}
	virtual QIcon icon() const
	{
		return QIcon();
	}
	virtual Scene * createScene() const
	{
		return new CubeScene();
	}
};

REGISTER_SCENE_FACTORY(CubeSceneFactory);



class ObjScene : public Scene
{
public:
	ObjScene(): m_dlistBase(0), m_dlistCount(0)
	{
		QString fileName = QFileDialog::getOpenFileName(NULL, QObject::tr("Open File"),
			SceneFactory::lastFile(), QString(QObject::tr("OBJ Files (%1)")).arg("*.obj"));

		if( !fileName.isEmpty() ) {
			load( fileName );
			SceneFactory::setLastFile( fileName );
		}		
	}
	
	virtual void transform() const
	{
		glScalef(m_scale, m_scale, m_scale);
		glTranslatef(-m_center.x, -m_center.y, -m_center.z);
	}
	
	virtual void draw(Effect* effect) const
	{
		GLuint lastList = m_dlistBase + m_dlistCount;  
		for (GLuint n = m_dlistBase; n < lastList; n++) {
			effect->beginMaterialGroup();
			glCallList(n);
		}
	}
	
	virtual void setupMenu(QMenu * menu) const
	{
		Q_UNUSED(menu);
	}
	
private:
	struct vec4 {
		vec4() { }
		vec4(float _x, float _y, float _z, float _w = 1.0f): x(_x), y(_y), z(_z), w(_w) { }
		
		float x, y, z, w;
	};
	
	struct vec3 {
		vec3() { }
		vec3(float _x, float _y, float _z): x(_x), y(_y), z(_z) { }
		
		float x, y, z;
	};
	
	struct vec2 {
		vec2() { }
		vec2(float _s, float _t): s(_s), t(_t) { }
		
		float s, t;
	};
	
	struct Material 
	{
		QString name;
		vec4 ka, kd, ks;
		float ns;
		
		Material(): ka(0.1f, 0.1f, 0.1f, 1.0f), kd(1.0f, 1.0f, 1.0f, 1.0f), ks(0.0f, 0.0f, 0.0f, 0.0f), ns(20.0f)		 
		{ }
		
		void bind() 
		{
			glMaterialfv(GL_FRONT, GL_AMBIENT, (GLfloat*)&ka);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, (GLfloat*)&kd);
			glMaterialfv(GL_FRONT, GL_SPECULAR, (GLfloat*)&ks);
			glMaterialf(GL_FRONT, GL_SHININESS, (GLfloat)ns);
		}
	};
	
	struct Vertex
	{
		int pos;
		int normal;
		int texcoord;
	};
	
	struct Surface 
	{
		Material* material;
		QVector< QVector<Vertex> > faces;
	};
	
	static float min(float a, float b) 
	{
		return a < b ? a : b;
	}
	
	static float max(float a, float b) 
	{
		return a > b ? a : b;
	}
	
	void load(const QString & fileName)
	{
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return;
		
		if (m_dlistBase)
			glDeleteLists(m_dlistBase, m_dlistCount);
		
		QRegExp vertexPattern("^v\\s+(.*)\\s+(.*)\\s+(.*)");
		QRegExp normalPattern("^vn\\s+(.*)\\s+(.*)\\s+(.*)");
		QRegExp texcoordPattern("^vt\\s+(.*)\\s+(.*)(\\s+.*)?");
		QRegExp mtllibPattern("^mtllib\\s+(.*\\.mtl)");
		QRegExp usemtlPattern("^usemtl\\s+(.*)(\\#.*)?");
		
		QVector<vec3> vertices, normals;
		QVector<vec2> texcoords;
		QVector<Material*> materials;
		QVector<Surface*> surfaces;
		
		vec3 vmin(1e10, 1e10, 1e10), vmax(-1e10, -1e10, -1e10);
		
		Material * defaultMaterial = new Material; 
		
		Surface * currentSurface = new Surface;
		currentSurface->material = defaultMaterial;
		surfaces.append(currentSurface);
		
		while (!file.atEnd()) {
			QString line = file.readLine().simplified();
			
			if (line.isEmpty() || line.startsWith('#'))
				continue;
			
			if (line.contains(vertexPattern)) {
				vec3 v(vertexPattern.cap(1).toFloat(), vertexPattern.cap(2).toFloat(), vertexPattern.cap(3).toFloat());
				vertices.append(v);
				vmin.x = min(v.x, vmin.x);
				vmin.y = min(v.y, vmin.y);
				vmin.z = min(v.z, vmin.z);
				
				vmax.x = max(v.x, vmax.x);
				vmax.y = max(v.y, vmax.y);
				vmax.z = max(v.z, vmax.z);				
			}
			
			else if (line.contains(normalPattern))
				normals.append(vec3(normalPattern.cap(1).toFloat(), normalPattern.cap(2).toFloat(), normalPattern.cap(3).toFloat()));
			
			else if (line.contains(texcoordPattern))
				texcoords.append(vec2(texcoordPattern.cap(1).toFloat(), texcoordPattern.cap(2).toFloat()));
			
			else if (line.startsWith("f ")) {
				QStringList faces = line.split(' ');
				
				QVector<Vertex> face;
				
				for (int i = 1; i < faces.size(); i++) {
					QStringList indices = faces[i].split('/');
					
					Vertex v;
					
					v.pos = indices[0].toInt() -1;
					if (v.pos < 0) v.pos += vertices.size() +1;
					Q_ASSERT(v.pos >= 0 && v.pos < vertices.size());
					
					if (indices.size() > 1 && !indices[1].isEmpty()) {
						v.texcoord = indices[1].toInt() -1;
						if (v.texcoord < 0) v.texcoord += texcoords.size() +1;
						Q_ASSERT(v.texcoord >= 0 && v.texcoord < texcoords.size());
					}
					else
						v.texcoord = -1;
					
					if (indices.size() > 2) {
						v.normal = indices[2].toInt() -1;
						if (v.normal < 0) v.normal += normals.size() +1;
						Q_ASSERT(v.normal >= 0 && v.normal < normals.size()); 
					}
					else
						v.normal = -1;
					
					face.append(v);
				}
				
				currentSurface->faces.append(face);
			}
			
			else if (line.contains(usemtlPattern)) {
				QString name = usemtlPattern.cap(1);
				
				currentSurface = NULL;
				foreach (Surface* surf, surfaces) {
					Q_ASSERT(surf != NULL);
					Q_ASSERT(surf->material != NULL);
					
					if (surf->material->name == name) {
						currentSurface = surf;
						break;
					}
				}
				
				if (!currentSurface) {
					Material* material = defaultMaterial;
					foreach (Material* mat, materials) {
						Q_ASSERT(mat != NULL);
						if (mat->name == name) {
							material = mat;
							break;
						}
					}
					
					currentSurface = new Surface;
					currentSurface->material = material;
					surfaces.append(currentSurface);
				}				
			}
			
			else if (line.contains(mtllibPattern)) {
				QFileInfo info(fileName);
				QFile mtlFile(info.dir().filePath(mtllibPattern.cap(1)));
				
				if (!mtlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
					qWarning("Could not open material file: %s", qPrintable(mtllibPattern.cap(1)));
					continue;
				}
				
				QRegExp newmtlPattern("^newmtl\\s(.*)(\\#.*)?");
				QRegExp kaPattern("^Ka\\s(.*)\\s(.*)\\s(.*)");
				QRegExp kdPattern("^Kd\\s(.*)\\s(.*)\\s(.*)");
				QRegExp ksPattern("^Ks\\s(.*)\\s(.*)\\s(.*)");
				QRegExp nsPattern("^Ns\\s(.*)");
				
				// @@ Existing surfaces are pointing to the materials that are being deleted!
				qDeleteAll(materials);
				materials.clear();
				
				Material* current = NULL;
				while (!mtlFile.atEnd()) {
					QString line = mtlFile.readLine().simplified();
					
					if (line.contains(newmtlPattern)) {
						current = new Material;
						materials.append(current);
						current->name = newmtlPattern.cap(1);
					}
					else if (line.contains(kaPattern)) {
						Q_ASSERT(current);
						current->ka = vec4(kaPattern.cap(1).toFloat(), kaPattern.cap(2).toFloat(), kaPattern.cap(3).toFloat());
					}
					else if (line.contains(kdPattern)) {
						Q_ASSERT(current);
						current->kd = vec4(kdPattern.cap(1).toFloat(), kdPattern.cap(2).toFloat(), kdPattern.cap(3).toFloat());
					}
					else if (line.contains(ksPattern)) {
						Q_ASSERT(current);
						current->ks = vec4(ksPattern.cap(1).toFloat(), ksPattern.cap(2).toFloat(), ksPattern.cap(3).toFloat());
					}
					else if (line.contains(nsPattern)) {
						Q_ASSERT(current);
						current->ns = nsPattern.cap(1).toFloat();
					}	
				}
			}
		}
		
		
		m_center.x = (vmax.x + vmin.x) * 0.5;
		m_center.y = (vmax.y + vmin.y) * 0.5;
		m_center.z = (vmax.z + vmin.z) * 0.5;
		
		m_scale = 1.0f / max(vmax.x - m_center.x, max(vmax.y - m_center.y, vmax.z - m_center.z));
		
		
		// create display lists
		if (surfaces[0]->faces.isEmpty())
			surfaces.remove(0);
		m_dlistCount = surfaces.size();
		m_dlistBase = glGenLists(m_dlistCount);
		
		for (int n = 0; n < m_dlistCount; n++) {
			glNewList(m_dlistBase + n, GL_COMPILE);			
			Surface* surf = surfaces[n];			
			surf->material->bind();
			
			for (int face = 0; face < surf->faces.count(); face++) {
				QVector<Vertex> verts = surf->faces[face];
				
				glBegin(GL_POLYGON);
				for (int vert = 0; vert < verts.count(); vert++) {
					
					if (verts[vert].texcoord >= 0)
						glTexCoord2fv((GLfloat*)&texcoords[verts[vert].texcoord]);
					
					if (verts[vert].normal >= 0)
						glNormal3fv((GLfloat*)&normals[verts[vert].normal]);
					
					glVertex3fv((GLfloat*)&vertices[verts[vert].pos]);
				}
				glEnd();
			}
			
			glEndList();
		}		
		
		delete defaultMaterial;
		qDeleteAll(materials);
		qDeleteAll(surfaces);
	}
	
	vec3 m_center;
	float m_scale;
	
	GLuint m_dlistBase, m_dlistCount;
};

// Obj scene factory.
class ObjSceneFactory : public SceneFactory
{
public:
	virtual QString name() const
	{
		return tr("OBJ file");
	}
	virtual QString description() const
	{
		return tr("WaveFront 3D Object");
	}
	virtual QIcon icon() const
	{
		return QIcon();
	}
	virtual Scene * createScene() const
	{
		return new ObjScene();
	}
};

REGISTER_SCENE_FACTORY(ObjSceneFactory);



// SceneFactory

namespace {
	static QList<const SceneFactory *> * s_factoryList = NULL;
	static QString s_lastFile;
}


//static
const SceneFactory * SceneFactory::findFactory(const QString & name)
{
	foreach(const SceneFactory * factory, factoryList()) {
		if( factory->name() == name ) {
			return factory;
		}
	}
}

//static
const QList<const SceneFactory *> & SceneFactory::factoryList()
{
	static QList<const SceneFactory *> empty;
	if(s_factoryList == NULL) {
		return empty;
	}
	else {
		return *s_factoryList;
	}
}

//static 
void SceneFactory::addFactory(const SceneFactory * factory)
{
	Q_ASSERT(factory != NULL);
	if( s_factoryList == NULL ) {
		s_factoryList = new QList<const SceneFactory *>();
	}
	s_factoryList->append(factory);
}

//static
void SceneFactory::removeFactory(const SceneFactory * factory)
{
	Q_ASSERT(factory != NULL);

	const int index = s_factoryList->lastIndexOf(factory);
	Q_ASSERT(index != -1);
	s_factoryList->removeAt(index);
	
	if( s_factoryList->isEmpty() ) {
		delete s_factoryList;
		s_factoryList = NULL;
	}
}

//static
Scene * SceneFactory::defaultScene()
{
	return new TeapotScene();
}

//static
const QString & SceneFactory::lastFile()
{
	return s_lastFile;
}

//static 
void SceneFactory::setLastFile(const QString & lastFile)
{
	s_lastFile = lastFile;
}
