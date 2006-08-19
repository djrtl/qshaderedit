
#include "effect.h"
#include "messagepanel.h"
#include "outputparser.h"
#include "texmanager.h"

#include <QtCore/QFile>
#include <QtCore/QByteArray>
#include <QtCore/QTime>
#include <QtCore/QVariant>
#include <QtCore/QHash>
#include <QtCore/QLibrary>

#include <Cg/cg.h>
#include <Cg/cgGL.h>

namespace {

	// Default vertex shader.
	static const char * s_effectText =
		"float3 color : DIFFUSE < string UIHelp = \"Base Color\"; > = {1.0f, 0.0f, 0.0f};\n"
		"float amb < string UIHelp = \"Ambient\"; > = 0.1f;\n\n"
		"float4x4 mvp : ModelViewProjection;\n"
		"float3x3 normalMatrix : ModelViewInverseTranspose;\n\n"
		"struct VertexInput {\n"
		"	float4 position : POSITION;\n"
		"	float3 normal : NORMAL;\n"
		"};\n\n"
		"struct VertexOutput {\n"
		"	float4 position : POSITION;\n"
		"	float3 normal : TEXCOORD0;\n"
		"};\n\n"
		"VertexOutput vp(VertexInput input)\n"
		"{\n"
		"	VertexOutput output;\n"
		"	output.position = mul(mvp, input.position);\n"
		"	output.normal = mul(normalMatrix, input.normal);\n"
		"	return output;\n"
		"}\n\n"
		"float4 fp(VertexOutput input) : COLOR\n"
		"{\n"
		"	float3 N = normalize(input.normal);\n"
		"	float3 L = normalize(gl_LightSource[0].position.xyz);\n"
		"	float3 H = normalize(gl_LightSource[0].halfVector.xyz);\n\n"
		"	float3 ambient = color * amb;\n"
		"	float3 diffuse = color * (1.0 - amb) * saturate(dot(L, N));\n"
		"	float3 specular = pow(saturate(dot(H, N)), 16.0);\n\n"
		"	return float4(ambient + diffuse + specular, 1.0);\n"
		"}\n\n"
		"technique main\n"
		"{\n"
		"	pass p0\n"
		"	{\n"
		"		VertexProgram = compile arbvp1 vp();\n"
		"		FragmentProgram = compile arbfp1 fp();\n"
		"		DepthTestEnable = true;\n"
		"		CullFaceEnable = true;\n"
		"	}\n"
		"}\n";

	struct CgSemantic
	{
		enum Type
		{
			Type_Null,
			Type_IdentityMatrix,
			Type_GLMatrix,
			Type_Time,
			Type_ViewportSize,
		//	Type_LightPosition,
		//	Type_LightDirection,
		//	Type_LightColor,
		};

		Type m_type;
		CGGLenum m_matrix;
		CGGLenum m_op;

		public:

			CgSemantic() : m_type(Type_Null)
			{
			}

			CgSemantic(Type type) : m_type(type)
			{
			}

			CgSemantic(CGGLenum matrix, CGGLenum op) : m_type(Type_GLMatrix), m_matrix(matrix), m_op(op)
			{
			}
	};

	static QHash<QString, CgSemantic> s_semanticMap;


	class CgParameter
	{
		private:
			CGparameter m_handle;
			QString m_name;
			QVariant m_value;
			bool m_visible;
			bool m_standard;
			Effect::EditorType m_editor;
			
			// Only for samplers.
			GLTexture m_tex;

		public:
			CgParameter() {}
			
			CgParameter(CGparameter h) : m_handle(h)
			{
				Q_ASSERT(h != NULL);
				m_name = cgGetParameterName(m_handle);
				m_value = getParameterValue(m_handle);
				m_visible = isParameterVisible(m_handle);
				m_standard = isParameterStandard(m_handle);
				m_editor = getParameterEditor(m_handle);
				parseAnnotations();
				
				// load texture.
				if( cgGetParameterClass(m_handle) == CG_PARAMETERCLASS_SAMPLER )
				{
					m_tex = GLTexture::open(m_value.toString());
				}
			}

			CGparameter handle() const
			{
				return m_handle;
			}
			
			const GLTexture & tex() const
			{
				return m_tex;
			}  

			const QString & name() const
			{
				return m_name;
			}

			const QVariant & value() const
			{
				return m_value;
			}
			void setValue(const QVariant & value)
			{
				m_value = value;
				
				if( cgGetParameterClass(m_handle) == CG_PARAMETERCLASS_SAMPLER )
				{
					m_tex = GLTexture::open(m_value.toString());
				}
			}

			Effect::EditorType editor() const
			{
				return m_editor;
			}

			int rows() const
			{
				return cgGetParameterRows(m_handle);
			}

			int columns() const
			{
				return cgGetParameterRows(m_handle);
			}

			bool isHidden() const
			{
				return !m_visible;
			}

			bool isStandard() const
			{
				return m_standard;
			}

		private:

			void parseAnnotations()
			{
				// Override UI name.
				CGannotation annotation = cgGetNamedParameterAnnotation(m_handle, "UIHelp");
				if(annotation != NULL) {
					m_name = cgGetStringAnnotationValue(annotation);
				}
			}

			static bool isParameterVisible(CGparameter parameter)
			{
				CGparameterclass parameterClass = cgGetParameterClass(parameter);

				Q_ASSERT(parameterClass != CG_PARAMETERCLASS_STRUCT);
				Q_ASSERT(parameterClass != CG_PARAMETERCLASS_ARRAY);

				if(parameterClass == CG_PARAMETERCLASS_UNKNOWN) return false;	// hide unknowns.
				if(parameterClass == CG_PARAMETERCLASS_OBJECT) return false;	// string and texture are not tweakable.

				// hide parameters without widget.
				CGannotation annotation = cgGetNamedParameterAnnotation(parameter, "UIWidget");
				if(annotation != NULL) {
					if(qstricmp("None", cgGetStringAnnotationValue(annotation)) == 0) {
						return false;
					}
				}

				return true;
			}

			static bool isParameterStandard(CGparameter parameter)
			{
				const char * semantic = cgGetParameterSemantic(parameter);
				if(semantic == NULL) return false;

				return s_semanticMap.contains(QString(semantic).toLower());
			}

			static QVariant getParameterValue(CGparameter parameter)
			{
				CGtype parameterBaseType = cgGetParameterBaseType(parameter);
				CGparameterclass parameterClass = cgGetParameterClass(parameter);

				Q_ASSERT(parameterClass != CG_PARAMETERCLASS_STRUCT);
				Q_ASSERT(parameterClass != CG_PARAMETERCLASS_ARRAY);

				if(parameterClass == CG_PARAMETERCLASS_OBJECT) {
					if(parameterBaseType == CG_STRING) {
						return cgGetStringParameterValue(parameter);
					}
					else if(parameterBaseType == CG_TEXTURE) {
						// Ignore textures.
					}
				}
				else if(parameterClass == CG_PARAMETERCLASS_SAMPLER) {
					// Get ResourceName annotation.
					CGannotation annotation = cgGetNamedParameterAnnotation(parameter, "ResourceName");
					if(annotation == 0) {
						// @@ Hack! state assignment name must be lowercase!
						CGstateassignment sa = cgGetNamedSamplerStateAssignment(parameter, "texture");
						if(sa != 0) {
							CGparameter p = cgGetTextureStateAssignmentValue(sa);
							if(p != 0) {
								annotation = cgGetNamedParameterAnnotation(p, "ResourceName");
							}
						}
					}

					// Get resource string.
					if(annotation == 0) {
						return QString();
					}
					else {
						return cgGetStringAnnotationValue(annotation);
					}
				}
				else if(parameterClass == CG_PARAMETERCLASS_SCALAR) {
					int num;
					const double * values = cgGetParameterValues(parameter, CG_DEFAULT, &num);
					Q_ASSERT(num == 1);

					if(parameterBaseType == CG_FLOAT || parameterBaseType == CG_HALF || parameterBaseType == CG_FIXED) {
						return values[0];
					}
					else if(parameterBaseType == CG_INT) {
						return int(values[0]);
					}
					else if(parameterBaseType == CG_BOOL) {
						return bool(values[0]);
					}
				}
				else if(parameterClass == CG_PARAMETERCLASS_VECTOR || parameterClass == CG_PARAMETERCLASS_MATRIX) {
					int num;
					const double * values = cgGetParameterValues(parameter, CG_DEFAULT, &num);
					Q_ASSERT(num > 0);

					QVariantList list;
					if(parameterBaseType == CG_FLOAT || parameterBaseType == CG_HALF || parameterBaseType == CG_FIXED) {
						for(int i = 0; i < num; i++) {
							list.append(float(values[i]));
						}
					}
					else if(parameterBaseType == CG_INT) {
						for(int i = 0; i < num; i++) {
							list.append(int(values[i]));
						}
					}
					else if(parameterBaseType == CG_BOOL) {
						for(int i = 0; i < num; i++) {
							list.append(bool(values[i]));
						}
					}
					return list;
				}
				return QVariant();
			}

			static Effect::EditorType getParameterEditor(CGparameter parameter)
			{
				CGparameterclass parameterClass = cgGetParameterClass(parameter);
				Q_ASSERT(parameterClass != CG_PARAMETERCLASS_STRUCT);
				Q_ASSERT(parameterClass != CG_PARAMETERCLASS_ARRAY);

				if(parameterClass == CG_PARAMETERCLASS_SCALAR) {
					return Effect::EditorType_Scalar;
				}
				else if(parameterClass == CG_PARAMETERCLASS_VECTOR) {
					const char * semantic = cgGetParameterSemantic(parameter);
					if( semantic != NULL ) {
						if( qstricmp("Diffuse", semantic) == 0 ||
							qstricmp("Specular", semantic) == 0)
						{
							return Effect::EditorType_Color;
						}
					}
					return Effect::EditorType_Vector;
				}
				else if(parameterClass == CG_PARAMETERCLASS_MATRIX) {
					return Effect::EditorType_Matrix;
				}
				else if(parameterClass == CG_PARAMETERCLASS_SAMPLER) {
					return Effect::EditorType_File;
				}

				return Effect::EditorType_None;
			}

	};

} // namespace



class CgEffect : public Effect
{
private:

	CGcontext m_context;
	CGeffect m_effect;
	CGtechnique m_technique;
	CGpass m_pass;

	QByteArray m_effectText;

	bool m_animated;
	QTime m_time;

	QVector<CgParameter> m_parameterArray;
	QVector<CgParameter> m_oldParameterArray;
	
	QList<CGtechnique> m_techniqueList;
	QList<CGpass> m_passList;

	OutputParser* m_outputParser;


public:

	CgEffect(const EffectFactory * factory) : Effect(factory),
		m_context(NULL),
		m_effect(NULL),
		m_technique(NULL),
		m_pass(NULL),
		m_effectText(s_effectText),
		m_animated(false)
	{
		m_context = cgCreateContext();

		cgGLSetManageTextureParameters(m_context, true);
		cgGLRegisterStates(m_context);

		if( s_semanticMap.empty() ) {
			// Cg is broken and transposes all the matrices.
			s_semanticMap.insert("worldviewprojection", CgSemantic(CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_TRANSPOSE));
			s_semanticMap.insert("worldviewprojectiontranspose", CgSemantic(CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY));
			s_semanticMap.insert("worldviewprojectioninverse", CgSemantic(CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_INVERSE_TRANSPOSE));
			s_semanticMap.insert("worldviewprojectioninversetranspose", CgSemantic(CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_INVERSE));

			s_semanticMap.insert("modelviewprojection", CgSemantic(CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_TRANSPOSE));
			s_semanticMap.insert("modelviewprojectiontranspose", CgSemantic(CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY));
			s_semanticMap.insert("modelviewprojectioninverse", CgSemantic(CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_INVERSE_TRANSPOSE));
			s_semanticMap.insert("modelviewprojectioninversetranspose", CgSemantic(CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_INVERSE));

			s_semanticMap.insert("modelview", CgSemantic(CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_TRANSPOSE));
			s_semanticMap.insert("modelviewtranspose", CgSemantic(CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_IDENTITY));
			s_semanticMap.insert("modelviewinverse", CgSemantic(CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE_TRANSPOSE));
			s_semanticMap.insert("modelviewinversetranspose", CgSemantic(CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE));

			s_semanticMap.insert("view", CgSemantic(CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_TRANSPOSE));
			s_semanticMap.insert("viewtranspose", CgSemantic(CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_IDENTITY));
			s_semanticMap.insert("viewinverse", CgSemantic(CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE_TRANSPOSE));
			s_semanticMap.insert("viewinversetranspose", CgSemantic(CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE));

			s_semanticMap.insert("world", CgSemantic(CgSemantic::Type_IdentityMatrix));
			s_semanticMap.insert("worldtranspose", CgSemantic(CgSemantic::Type_IdentityMatrix));
			s_semanticMap.insert("worldinverse", CgSemantic(CgSemantic::Type_IdentityMatrix));
			s_semanticMap.insert("worldinversetranspose", CgSemantic(CgSemantic::Type_IdentityMatrix));

			s_semanticMap.insert("projection", CgSemantic(CG_GL_PROJECTION_MATRIX, CG_GL_MATRIX_TRANSPOSE));
			s_semanticMap.insert("projectiontranspose", CgSemantic(CG_GL_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY));

			s_semanticMap.insert("time", CgSemantic(CgSemantic::Type_Time));
			s_semanticMap.insert("viewportsize", CgSemantic(CgSemantic::Type_ViewportSize));
		}

		m_outputParser = new NvidiaOutputParser;

		m_time.start();
	}

	virtual ~CgEffect()
	{
		cgDestroyContext(m_context);
		delete m_outputParser;
	}


	// Load/Save the effect.
	virtual void load(QFile * file)
	{
		Q_ASSERT(file != NULL);

		m_effectText = file->readAll();

		m_time.start();
	}

	virtual void save(QFile * file)
	{
		Q_ASSERT(file != NULL);

		file->write(m_effectText);
	}

	virtual int getInputNum()
	{
		return 1;
	}

	virtual QString getInputName(int i)
	{
		Q_ASSERT(i == 0);
		return tr("Effect");
	}

	virtual const QByteArray & getInput(int i) const
	{
		Q_ASSERT(i == 0);
		return m_effectText;
	}

	virtual void setInput(int i, const QByteArray & str)
	{
		Q_ASSERT(i == 0);
		m_effectText = str;
	}

	// Compilation.
	virtual bool build(MessagePanel * output)
	{
		freeEffect();

		if(output != NULL) output->clear();

		if(output != NULL) output->info(tr("Compiling cg effect..."));

		m_effect = cgCreateEffect(m_context, m_effectText.data(), NULL);

		// Output compilation errors.
		if (output != NULL) output->log(cgGetLastListing(m_context), 0, m_outputParser);

		if (m_effect == NULL)
		{
			return false;
		}

		// Read and validate techniques.
		CGtechnique technique = cgGetFirstTechnique(m_effect);
		while(technique != NULL)
		{
			if (cgValidateTechnique(technique))
			{
				const char * name = cgGetTechniqueName(technique);
				if (output != NULL) output->info(tr("Validated technique '%1'").arg(name));
				m_techniqueList.append(technique);
			}

			// Output validation errors.
			if (output != NULL) output->log(cgGetLastListing(m_context), 0, m_outputParser);

			technique = cgGetNextTechnique(technique);
		}

		if (m_techniqueList.count() == 0)
		{
			return false;
		}
		selectTechnique(0);

	//	error = cgGetError();
	//	if( error != CG_NO_ERROR ) {
	//		printf("%s\n", cgGetErrorString(error));
	//	}

		initParameters();

		return true;
	}

	// Parameter info.
	virtual int getParameterNum() const
	{
		return m_parameterArray.count();
	}
	virtual QString getParameterName(int idx) const
	{
		Q_ASSERT(idx < getParameterNum());
		return m_parameterArray.at(idx).name();
	}
	virtual QVariant getParameterValue(int idx) const
	{
		Q_ASSERT(idx < getParameterNum());
		return m_parameterArray.at(idx).value();
	}
	virtual void setParameterValue(int idx, const QVariant & value)
	{
		Q_ASSERT(idx < getParameterNum());
		m_parameterArray[idx].setValue(value);
	}
	virtual EditorType getParameterEditor(int idx) const
	{
		Q_ASSERT(idx < getParameterNum());
		return m_parameterArray[idx].editor();
	}
	virtual int getParameterRows(int idx) const
	{
		return m_parameterArray.at(idx).rows();
	}
	virtual int getParameterColumns(int idx) const
	{
		return m_parameterArray.at(idx).columns();
	}

	// Effect info.
	virtual bool isValid() const
	{
		return m_effect != 0 && m_technique != 0;
	}

	virtual bool isAnimated() const
	{
		return m_animated;
	}

	// Technique info.
	virtual int getTechniqueNum() const
	{
		return m_techniqueList.count();
	}
	virtual QString getTechniqueName(int t) const
	{
		Q_ASSERT(t <  getTechniqueNum());
		return cgGetTechniqueName(m_techniqueList.at(t));
	}
	virtual void selectTechnique(int t)
	{
		Q_ASSERT(t <  getTechniqueNum());
		m_technique = m_techniqueList.at(t);

		m_passList.clear();

		CGpass pass = cgGetFirstPass(m_technique);
		while(pass != NULL)
		{
			m_passList.append(pass);
			pass = cgGetNextPass(pass);
		}
	}

	// Pass info.
	virtual int getPassNum() const
	{
		Q_ASSERT(m_technique != NULL);
		return m_passList.count();
	}

	// Rendering.
	virtual void begin()
	{
		Q_ASSERT(m_technique != NULL);
		Q_ASSERT(m_pass == NULL);

		// Set standard parameter values.
		CGparameter parameter = cgGetFirstLeafEffectParameter(m_effect);
		while(parameter != NULL) {
			if(cgIsParameterUsed(parameter, m_effect)) {
				QString semantic = cgGetParameterSemantic(parameter);
				semantic = semantic.toLower();

				if( semantic != NULL && s_semanticMap.contains(semantic) ) {
					const CgSemantic & std = s_semanticMap[semantic];

					if( std.m_type == CgSemantic::Type_GLMatrix ) {
						cgGLSetStateMatrixParameter(parameter, std.m_matrix, std.m_op);
					}
					else if( std.m_type == CgSemantic::Type_IdentityMatrix ) {
						// @@ ???
					}
					else if( std.m_type == CgSemantic::Type_Time ) {
						cgSetParameter1f(parameter, float(m_time.elapsed()));
					}
					else if( std.m_type == CgSemantic::Type_ViewportSize ) {
						GLfloat v[4];
						glGetFloatv(GL_VIEWPORT, v);
						cgSetParameter2f(parameter, v[2], v[3]);
					}
				}
			}
			parameter = cgGetNextLeafParameter(parameter);
		}

		// Set user parameters.
		foreach(const CgParameter & p, m_parameterArray) {
			parameter = p.handle();
			QVariant value = p.value();

			// Set cg parameter.
			CGparameterclass parameterClass = cgGetParameterClass(parameter);
		//	CGtype parameterType = cgGetParameterType(parameter);
		//	CGtype parameterBaseType = cgGetParameterBaseType(parameter);

			if( parameterClass == CG_PARAMETERCLASS_SCALAR ) {
				if( value.type() == QVariant::Double ) {
					cgSetParameter1d(parameter, value.toDouble());
				}
				else if( value.type() == QVariant::Int ) {
					cgSetParameter1i(parameter, value.toInt());
				}
				else if( value.type() == QVariant::Bool ) {
					cgSetParameter1i(parameter, value.toBool());
				}
			}
			else if( parameterClass == CG_PARAMETERCLASS_VECTOR ) {
				Q_ASSERT(value.canConvert(QVariant::List));
				QVariantList list = value.toList();

				int size = cgGetParameterColumns(parameter);	// Cg assumes row vectors!
				Q_ASSERT(list.count() == size);

				switch(size) {
					case 1:
						cgSetParameter1d(parameter, list.at(0).toDouble());
						break;
					case 2:
						cgSetParameter2d(parameter, list.at(0).toDouble(), list.at(1).toDouble());
						break;
					case 3:
						cgSetParameter3d(parameter, list.at(0).toDouble(), list.at(1).toDouble(), list.at(2).toDouble());
						break;
					case 4:
						cgSetParameter4d(parameter, list.at(0).toDouble(), list.at(1).toDouble(), list.at(2).toDouble(), list.at(3).toDouble());
						break;
				}
			}
			else if( parameterClass == CG_PARAMETERCLASS_MATRIX ) {
				// @@ TBD
			}
			else if( parameterClass == CG_PARAMETERCLASS_SAMPLER ) {
				cgGLSetTextureParameter(parameter, p.tex().object());
			}
			else if( parameterClass == CG_PARAMETERCLASS_OBJECT ) {
				// Ignore textures and strings.
			}
		}
	}
	virtual void beginPass(int p)
	{
		Q_ASSERT(p <  getPassNum());
		m_pass = m_passList.at(p);
		cgSetPassState(m_pass);
	}
	virtual void endPass()
	{
		cgResetPassState(m_pass);
		m_pass = NULL;
	}
	virtual void end()
	{
		Q_ASSERT(m_pass == NULL);
	}

private:

	void resetParameters()
	{
		m_animated = false;
		
		qSwap(m_oldParameterArray, m_parameterArray);
		m_parameterArray.clear();
	}
	
	void initParameters()
	{
		resetParameters();
		
		// Read parameters.
		CGparameter parameter = cgGetFirstLeafEffectParameter(m_effect);
		while(parameter != NULL)
		{
			if(cgIsParameterUsed(parameter, m_effect))
			{
				CgParameter cgParameter(parameter);
				
				// Try to get the old value.
				foreach(const CgParameter & p, m_oldParameterArray) {
					if( p.name() == cgParameter.name() ) {
						cgParameter.setValue(p.value());
					}
				}					
				
				// Add non hidden and non standard parameters.
				if(!cgParameter.isHidden() && !cgParameter.isStandard()) {
					m_parameterArray.append(cgParameter);
				}
				
				// Process semantics of standard parameters.
				if( cgParameter.isStandard() ) {
					QString semantic = cgGetParameterSemantic(parameter);
					semantic = semantic.toLower();
					
					if( s_semanticMap.contains(semantic) ) {
						const CgSemantic & std = s_semanticMap[semantic];
						if( std.m_type == CgSemantic::Type_Time ) {
							m_animated = true;
						}
					}
				}
			}
			parameter = cgGetNextLeafParameter(parameter);
		}
	}
	
	void freeEffect()
	{
		if(m_effect != NULL) {
			cgDestroyEffect(m_effect);
			m_effect = NULL;
		}
		
		m_technique = NULL;
		m_pass = NULL;
		
		// resetParameters();
		
		m_techniqueList.clear();
		m_passList.clear();
	}

};

// @@ weak import everything...
//#if defined(Q_OS_DARWIN)
//extern "C" {
//	extern const char *cgGetString(CGenum sname) __attribute__((weak_import));
//}
//#endif


class CgFxEffectFactory : public EffectFactory
{
	virtual bool isSupported() const
	{

#if !defined(Q_OS_DARWIN)
		typedef const char * (* GetString)(CGenum sname);
		QLibrary cgLibrary("Cg");

		if( !cgLibrary.load() )
		{
			qDebug("Cg library not found.");
			return false;
		}

		GetString cgGetString = (GetString) cgLibrary.resolve("cgGetString");
#endif

		if(cgGetString != NULL)
		{
			QRegExp rx("^(\\d)\\.(\\d).*");
			QString version = cgGetString(CG_VERSION);

			if(rx.exactMatch(version))
			{
				int major = rx.cap(1).toInt();
				int minor = rx.cap(2).toInt();

				if((major == 1 && minor > 4) || major > 1)
				{
					return true;
				}
			}
			else
			{
				qDebug("cg version string not recognized.");
			}
		}

		return false;
	}

	virtual QString name() const
	{
		return tr("CgFx Effect");
	}

	virtual QString namePlural() const
	{
		return tr("CgFx Effects");
	}

	virtual QString extension() const
	{
		return "cgfx";
	}

	virtual QIcon icon() const
	{
		return QIcon();
	}

	virtual Effect * createEffect() const
	{
		Q_ASSERT(isSupported());
		return new CgEffect(this);
	}

	virtual QList<Highlighter::Rule> highlightingRules() const
	{
		QList<Highlighter::Rule> rules;
		Highlighter::Rule rule;

		rule.type = Highlighter::Keyword;
		rule.pattern = QRegExp("\\b(if|else|for|while|do|struct|break|continue|discard|return|technique|pass|sampler_state|"
			"compile|true|false|packed|typedef)\\b");
		rules.append(rule);

		rule.type = Highlighter::DataType;
		rule.pattern = QRegExp(
			"\\b(void|float|float[1-4]|float[1-4]x[1-4]|int|int[1-4]|int[1-4]x[1-4]|bool|bool[1-4]|bool[1-4]x[1-4]|"
			"sampler[1-3]D|samplerCUBE|samplerRECT|texture|string|uniform|varying|static|const|in|out|inout)\\b");
		rules.append(rule);

		rule.type = Highlighter::BuiltinVar;
		rule.pattern = QRegExp(
			"\\b(gl_(Position|PointSize|ClipVertex|FragCoord|FragFacing|FragColor|"
			"FragData|FragDepth|Color|SecondaryColor|Normal|Vertex|MultiTexCoord[0-7]|FogColor|"
			"MaxLights|MaxClipPlanes|MaxTextureUnits|MaxTextureCoords|MaxVertexAttributes|"
			"MaxVertexUniformComponents|MaxVaryingFloats|MaxVertexTextureImageUnits|MaxCombinedTextureImageUnits|"
			"MaxTextureImageUnits|MaxFragmentUniformComponents|MaxDrawBuffers|ModelViewMatrix|ProjectionMatrix|"
			"ModelViewProjectionMatrix|TextureMatrix|NormalMatrix|ModelViewMatrixInverse|ProjectionMatrixInverse|"
			"ModelViewProjectionMatrixInverse|TextureMatrixInverse|ModelViewMatrixTranspose|ProjectionMatrixTranspose|"
			"ModelViewProjectionMatrixTranspose|TextureMatrixTranspose|ModelViewMatrixInverseTranspose|ProjectionMatrixInverseTranspose|"
			"ModelViewProjectionMatrixInverseTranspose|TextureMatrixInverseTranspose|NormScale|DepthRangeParameters|DepthRange|"
			"ClipPlane|PointParameters|Point|MaterialParameters|FrontMaterial|BackMaterial|LightSourceParameters|LightSource|"
			"LightModelParameters|LightModel|LightModelProducts|FrontLightModelProduct|BackLightModelProduct|LightProducts|"
			"FrontLightProduct|BackLightProduct|TextureEnvColor|EyePlaneS|EyePlaneT|EyePlaneR|EyePlaneQ|ObjectPlaneS|ObjectPlaneT|"
			"ObjectPlaneR|ObjectPlaneQ|FogParameters|Fog|FrontColor|BackColor|FrontSecondaryColor|BackSecondaryColor|TexCoord|FogFragCoord|Color|"
			"SecondaryColor)|"
			"WorldViewProjection(Inverse)?(Transpose)?|ModelView(Projection)?(Inverse)?(Transpose)?|View(Inverse)?(Transpose)?|"
			"World(Inverse)?(Transpose)?|Projection(Inverse)?(Transpose)?|Time|"
			"VertexProgram|FragmentProgram|DepthTestEnable|CullFaceEnable|register\\(c[1-2]+\\))\\b");
		rules.append(rule);

		rule.type = Highlighter::BuiltinFunction;
		rule.pattern = QRegExp(
			"\\b(abs|acos|all|any|asin|atan|atan2|ceil|clamp|cos|cosh|cross|degrees|determinant|dot|length|"
			"max|min|radians|reflect|refract|round|saturate|sin|sinh|tan|tanh|transpose|"
			"lerp|tex1D(proj)?|tex2D(proj)?|texRECT(proj)?|tex3D(proj)?|texCUBE(proj)?|"
			"offsettex2D|offsettexRECT|offsettex2DScaleBias|offsettexRECTScaleBias|tex1D_dp3|tex2D_dp3x2|"
			"texRECT_dp3x2|tex3D_dp3x3|texCUBE_dp3x3|texCUBE_reflect_dp3x3|texCUBE_reflect_eye_dp3x3|tex_dp3x2_depth|"
			"(un)?pack_4(u)?byte|(un)?pack_2ushort|(un)?pack_2half)\\b");
		rules.append(rule);
		
		
		rule.type = Highlighter::Number;
		rule.pattern = QRegExp("\\b[-+]?\\d*\\.?\\d+([eE][-+]?\\d+)?[fFhHxX]?\\b");
		rules.append(rule);

		rule.type = Highlighter::String;
		rule.pattern = QRegExp("\".*\"");
		rules.append(rule);

		rule.type = Highlighter::Misc;
		rule.pattern = QRegExp("#.*(//|$)");
		rules.append(rule);

		rule.type = Highlighter::Comment;
		rule.pattern = QRegExp("//.*$");
		rules.append(rule);

		return rules;
	}

	QString multiLineCommentStart() const
	{
		return "/*";
	}

	QString multiLineCommentEnd() const
	{
		return "*/";
	}
};

REGISTER_EFFECT_FACTORY(CgFxEffectFactory);

