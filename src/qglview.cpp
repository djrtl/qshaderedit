
#include "qglview.h"
#include "effect.h"
#include "messagepanel.h"
#include "texmanager.h"
#include "scene.h"

#include <math.h>

#include <QtCore/QUrl>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QMenu>
#include <QtGui/QAction>


QGLView::QGLView(const QGLFormat & format, QWidget *parent) : QGLWidget(format, parent),
	m_effect(NULL), m_scene(NULL)
{
    setAcceptDrops(true);
	
	m_wireframeAction = new QAction(tr("Wireframe"), this);
	m_wireframeAction->setCheckable(true);
	m_wireframeAction->setChecked(false);
	connect(m_wireframeAction, SIGNAL(toggled(bool)), this, SLOT(settingsChanged()));
	
	m_orthoAction = new QAction(tr("Ortho"), this);
	m_orthoAction->setCheckable(true);
	m_orthoAction->setChecked(false);
	connect(m_orthoAction, SIGNAL(toggled(bool)), this, SLOT(settingsChanged()));
}


QGLView::~QGLView()
{
	delete m_wireframeAction;
	delete m_orthoAction;
}


void QGLView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

void QGLView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    event->acceptProposedAction();
	if( urls.size() ) {
        emit fileDropped( urls[0].toLocalFile() );
	}
}

QSize QGLView::sizeHint() const
{
	return QSize(200, 200);
}
QSize QGLView::minimumSizeHint() const
{
	return QSize(100, 100);
}


void QGLView::setEffect(Effect * effect)
{
	m_effect = effect;
	if( m_effect != NULL ) {
		makeCurrent();
	}
}

void QGLView::resetEffect()
{
	m_effect = NULL;
}

void QGLView::setScene(Scene * scene)
{
	if( m_scene != NULL ) {
		delete m_scene;
	}
	m_scene = scene;
	resetTransform();
	updateGL();
}

void QGLView::populateMenu(QMenu * menu)
{
	Q_ASSERT(menu != NULL);
	
	// Add common actions.
	menu->addAction(m_wireframeAction);
	menu->addAction(m_orthoAction);
		
	if( m_scene != NULL ) {
		// @@ Add scene actions.
	}
}


bool QGLView::init(MessagePanel * output)
{
	Q_ASSERT(output != NULL);
	
	makeCurrent();
	setAutoBufferSwap(false);
	
	// @@ Move to initializeGL
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		//fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		return false;
	}
	
	const char * vendor = (const char *)glGetString(GL_VENDOR);
	output->info(QString("OpenGL vendor: ").append(vendor));
	
	const char * renderer = (const char *)glGetString(GL_RENDERER);
	output->info(QString("OpenGL renderer: ").append(renderer));
		
	const char * version = (const char *)glGetString(GL_VERSION);
	output->info(QString("OpenGL version: ").append(version));
	
	output->info(QString("OpenGL extensions:"));
	
	if( GLEW_ARB_vertex_shader ) output->info("- ARB_vertex_shader FOUND");
	else output->error("- ARB_vertex_shader NOT FOUND");
	
	if( GLEW_ARB_fragment_shader ) output->info("- ARB_fragment_shader FOUND");
	else output->error("- ARB_fragment_shader NOT FOUND");
	
	if( GLEW_ARB_shader_objects ) output->info("- ARB_shader_objects FOUND");
	else output->error("- ARB_shader_objects NOT FOUND");
	
	if( GLEW_ARB_shading_language_100 ) output->info("- ARB_shading_language_100 FOUND");
	else output->error("- ARB_shading_language_100 NOT FOUND");
	
	if( GLEW_VERSION_2_0 ) {
		const char * glsl_version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
		output->info(QString("GLSL version: ").append(glsl_version));
	}
	
	// Set special settings for mesa.
	if( qstrcmp(vendor, "Brian Paul") == 0 ) {
		glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST );
	}

	return true;
}

void QGLView::initializeGL()
{
	glClearColor (0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	
	m_alpha = 0.0f;
	m_beta = 0.0f;
	m_x = 0.0f;
	m_y = 0.0f;
	m_z = 5.0f;	

	m_scene = SceneFactory::defaultScene();
}

void QGLView::resetGL()
{
	delete m_scene;
}


void QGLView::resizeGL(int w, int h)
{
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	updateMatrices();
}

void QGLView::paintGL()
{
	if( !isVisible() ) {
		// @@ updateGL should not be called when the window is hidden!
		return;
	}
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	if( m_scene != NULL ) {
	
		if( m_effect != NULL && m_effect->isValid() ) {
		
			// Setup ligh parameters @@ Move this to scene->setup() or begin()
			float light_vector[4] = {1.2f/sqrt(3.08f), 1.0f/sqrt(3.08f), 0.8f/sqrt(3.08f), 0.0f};
			glLightfv( GL_LIGHT0, GL_POSITION, light_vector );
			
			if( m_wireframeAction->isChecked() ) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}
			else {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			
			m_effect->begin();
			
			for(int i = 0; i < m_effect->getPassNum(); i++) {
				m_effect->beginPass(i);
				
				m_scene->draw(m_effect);
				
				m_effect->endPass();
			}
			
			m_effect->end();
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			m_scene->draw(NULL);
		}
	}
	
 	swapBuffers();
	
	//qDebug("paint!");
}


void QGLView::updateMatrices()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	float aspect = float(width())/float(height());
	
	if( !m_orthoAction->isChecked() ) {
		gluPerspective(30, aspect, 0.5, 50);
	}
	else {
		glOrtho(-aspect,aspect, -1,1, -30,30);
		glScalef(m_z/5, m_z/5, m_z/5);
	}
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// World transform:
	gluLookAt(m_x, m_y, m_z, m_x, m_y, m_z-1, 0, 1, 0);
	glRotatef(m_beta, 1, 0, 0);
	glRotatef(m_alpha, 0, 1, 0);
	
	// Object transform:
	m_scene->transform();
}


void QGLView::mousePressEvent(QMouseEvent *event)
{
	m_pos = event->pos();
	m_button = event->button();
}

void QGLView::mouseMoveEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();

	if(m_button == Qt::LeftButton && event->modifiers() == Qt::NoModifier) {
		m_alpha += (240.0f * (pos - m_pos).x()) / height();
		m_beta += (240.0f * (pos - m_pos).y()) / height();
		if(m_beta < -90) m_beta = -90;
		else if(m_beta > 90) m_beta = 90;
	}
	else if(m_button == Qt::RightButton ||
		(m_button == Qt::LeftButton && event->modifiers() == Qt::ControlModifier)) 
	{
		m_x -= (4.0f * (pos - m_pos).x()) / height();
		m_y += (4.0f * (pos - m_pos).y()) / height();
	}
	else if(m_button == Qt::MidButton ||
		(m_button == Qt::LeftButton && event->modifiers() == Qt::ShiftModifier)) 
	{
		m_z -= (5.0f * (pos - m_pos).y()) / height();
	}

	m_pos = pos;

	updateMatrices();
	updateGL();
}

void QGLView::mouseReleaseEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	m_button = Qt::NoButton;
}

void QGLView::wheelEvent(QWheelEvent *e)
{
	m_z += (e->delta()/120.0)/10.0;
	updateMatrices();
	updateGL();
}

void QGLView::resetTransform()
{
	m_alpha = 0.0f;
	m_beta = 0.0f;
	m_x = 0.0f;
	m_y = 0.0f;
	m_z = 5.0f;
	updateMatrices();	
}

void QGLView::settingsChanged()
{
	updateMatrices();
	emit updateGL();
}
