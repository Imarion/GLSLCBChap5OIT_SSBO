#include "OIT_SSBO.h"

#include <QtGlobal>

#include <QDebug>
#include <QFile>
#include <QImage>
#include <QTime>

#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>

#include <cmath>
#include <cstring>
#include <sstream>
#include <vector>

MyWindow::~MyWindow()
{
    if (mProgram != 0) delete mProgram;
}

MyWindow::MyWindow()
    : mProgram(0), currentTimeMs(0), currentTimeS(0), tPrev(0), angle(M_PI / 2.0f)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(4);
    format.setMinorVersion(3);
    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
    create();

    resize(800, 600);

    mContext = new QOpenGLContext(this);
    mContext->setFormat(format);
    mContext->create();

    mContext->makeCurrent( this );

    mFuncs = mContext->versionFunctions<QOpenGLFunctions_4_3_Core>();
    if ( !mFuncs )
    {
        qWarning( "Could not obtain OpenGL versions object" );
        exit( 1 );
    }
    if (mFuncs->initializeOpenGLFunctions() == GL_FALSE)
    {
        qWarning( "Could not initialize core open GL functions" );
        exit( 1 );
    }

    initializeOpenGLFunctions();

    QTimer *repaintTimer = new QTimer(this);
    connect(repaintTimer, &QTimer::timeout, this, &MyWindow::render);
    repaintTimer->start(1000/60);

    QTimer *elapsedTimer = new QTimer(this);
    connect(elapsedTimer, &QTimer::timeout, this, &MyWindow::modCurTime);
    elapsedTimer->start(1);       
}

void MyWindow::modCurTime()
{
    currentTimeMs++;
    currentTimeS=currentTimeMs/1000.0f;
}

void MyWindow::initialize()
{
    angle = ToRadian(210);

    CreateVertexBuffer();
    initShaders();
    pass1Index = mFuncs->glGetSubroutineIndex( mProgram->programId(), GL_FRAGMENT_SHADER, "pass1");
    pass2Index = mFuncs->glGetSubroutineIndex( mProgram->programId(), GL_FRAGMENT_SHADER, "pass2");

    initMatrices();
    initShaderStorage();

    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void MyWindow::CreateVertexBuffer()
{
    // *** Sphere
    mFuncs->glGenVertexArrays(1, &mVAOSphere);
    mFuncs->glBindVertexArray(mVAOSphere);

    mSphere = new VBOSphere(1.0f, 40, 40);

    // Create and populate the buffer objects
    unsigned int SphereHandles[4];
    glGenBuffers(4, SphereHandles);

    glBindBuffer(GL_ARRAY_BUFFER, SphereHandles[0]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mSphere->getnVerts()) * sizeof(float), mSphere->getv(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, SphereHandles[1]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mSphere->getnVerts()) * sizeof(float), mSphere->getn(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, SphereHandles[2]);
    glBufferData(GL_ARRAY_BUFFER, (2 * mSphere->getnVerts()) * sizeof(float), mSphere->gettc(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, SphereHandles[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mSphere->getnFaces() * sizeof(unsigned int), mSphere->getelems(), GL_STATIC_DRAW);

    // Setup the VAO
    // Vertex positions
    mFuncs->glBindVertexBuffer(0, SphereHandles[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex normals
    mFuncs->glBindVertexBuffer(1, SphereHandles[1], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(1, 1);

    // vertex texture coord
    mFuncs->glBindVertexBuffer(2, SphereHandles[2], 0, sizeof(GLfloat) * 2);
    mFuncs->glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(2, 2);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, SphereHandles[3]);

    mFuncs->glBindVertexArray(0);

    // *** Cube
    mFuncs->glGenVertexArrays(1, &mVAOCube);
    mFuncs->glBindVertexArray(mVAOCube);

    mCube = new VBOCube();

    // Create and populate the buffer objects
    unsigned int CubeHandles[4];
    glGenBuffers(4, CubeHandles);

    glBindBuffer(GL_ARRAY_BUFFER, CubeHandles[0]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mCube->getnVerts()) * sizeof(float), mCube->getv(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, CubeHandles[1]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mCube->getnVerts()) * sizeof(float), mCube->getn(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, CubeHandles[2]);
    glBufferData(GL_ARRAY_BUFFER, (2 * mCube->getnVerts()) * sizeof(float), mCube->gettc(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CubeHandles[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * mCube->getnFaces() * sizeof(unsigned int), mCube->getel(), GL_STATIC_DRAW);

    // Setup the VAO
    // Vertex positions
    mFuncs->glBindVertexBuffer(0, CubeHandles[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex normals
    mFuncs->glBindVertexBuffer(1, CubeHandles[1], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(1, 1);

    // Vertex texure coordinates
    mFuncs->glBindVertexBuffer(2, CubeHandles[2], 0, sizeof(GLfloat) * 2);
    mFuncs->glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(2, 2);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CubeHandles[3]);

    mFuncs->glBindVertexArray(0);

    // *** Array for full-screen quad
    GLfloat verts[] = {
        -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f
    };
    GLfloat tc[] = {
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
    };

    // Set up the buffers

    unsigned int fsqhandle[2];
    glGenBuffers(2, fsqhandle);

    glBindBuffer(GL_ARRAY_BUFFER, fsqhandle[0]);
    glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), verts, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, fsqhandle[1]);
    glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), tc, GL_STATIC_DRAW);

    // Set up the VAO
    mFuncs->glGenVertexArrays( 1, &mVAOFSQuad );
    mFuncs->glBindVertexArray(mVAOFSQuad);

    // Vertex positions
    mFuncs->glBindVertexBuffer(0, fsqhandle[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex texture coordinates
    mFuncs->glBindVertexBuffer(1, fsqhandle[1], 0, sizeof(GLfloat) * 2);
    mFuncs->glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(2, 1);

    mFuncs->glBindVertexArray(0);

}

void MyWindow::initMatrices()
{
    /*
    ModelMatrixTeapot.translate( 3.0f, -5.0f, 1.5f);
    ModelMatrixTeapot.rotate( -90.0f, QVector3D(1.0f, 0.0f, 0.0f));

    ModelMatrixSphere.translate( -3.0f, -3.0f, 2.0f);

    ModelMatrixBackPlane.rotate(90.0f, QVector3D(1.0f, 0.0f, 0.0f));
    ModelMatrixBotPlane.translate(0.0f, -5.0f, 0.0f);
    ModelMatrixTopPlane.translate(0.0f,  5.0f, 0.0f);
    ModelMatrixTopPlane.rotate(180.0f, 1.0f, 0.0f, 0.0f);
    */
    //ModelMatrixPlane.translate(0.0f, -0.45f, 0.0f);

    ViewMatrix.lookAt(QVector3D(11.0f * cos(angle), 2.0f, 11.0f * sin(angle)), QVector3D(0.0f,0.0f,0.0f), QVector3D(0.0f,1.0f,0.0f));
}

void MyWindow::resizeEvent(QResizeEvent *)
{
    mUpdateSize = true;

    ProjectionMatrix.setToIdentity();
    ProjectionMatrix.perspective(50.0f, (float)this->width()/(float)this->height(), 1.0f, 1000.0f);
}

void MyWindow::render()
{
    if(!isVisible() || !isExposed())
        return;

    if (!mContext->makeCurrent(this))
        return;

    static bool initialized = false;
    if (!initialized) {
        initialize();
        initialized = true;
    }

    if (mUpdateSize) {
        glViewport(0, 0, size().width(), size().height());
        mUpdateSize = false;
    }

    /*
    float deltaT = currentTimeS - tPrev;
    if(tPrev == 0.0f) deltaT = 0.0f;
    tPrev = currentTimeS;
    angle += 0.25f * deltaT;
    if (angle > TwoPI) angle -= TwoPI;

    static float EvolvingVal = 0;
    EvolvingVal += 0.1f;
    */

    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    clearBuffers();
    pass1();
    pass2();

    mContext->swapBuffers(this);
}

void MyWindow::pass1()
{   
    glViewport(0, 0, this->width(), this->height());

    glClearColor(0.5f,0.5f,0.5f,1.0f);    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);    
    glDepthMask(GL_FALSE);

    DrawScene();
}

void MyWindow::DrawScene()
{
    // *** Draw spheres
    mFuncs->glBindVertexArray(mVAOSphere);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    mProgram->bind();
    {
        mFuncs->glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &pass1Index);

        mProgram->setUniformValue("Light.Position",  QVector4D(0.0f, 0.0f, 0.0f, 1.0f));
        mProgram->setUniformValue("Light.Intensity", QVector3D(0.9f, 0.9f, 0.9f));

        mProgram->setUniformValue("ViewNormalMatrix", ViewMatrix.normalMatrix());

        mProgram->setUniformValue("Material.Kd", 0.2f, 0.2f, 0.9f, 0.55f);
        mProgram->setUniformValue("Material.Ka", 0.0f, 0.0f, 0.0f, 0.0f);

        float size = 0.45f;
        for (int i=0; i<=6; i++)
        {
            for( int j=0; j<=6; j++)
            {
                for( int k=0; k<=6; k++)
                {
                    if( (i+j+k)%2 == 0)
                    {
                        ModelMatrixSphere.setToIdentity();
                        ModelMatrixSphere.translate(i-3, j-3, k-3);
                        ModelMatrixSphere.scale(size);

                        QMatrix4x4 mv1 = ViewMatrix * ModelMatrixSphere;
                        mProgram->setUniformValue("ModelViewMatrix", mv1);
                        mProgram->setUniformValue("NormalMatrix", mv1.normalMatrix());
                        mProgram->setUniformValue("MVP", ProjectionMatrix * mv1);

                        glDrawElements(GL_TRIANGLES, mSphere->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));
                    }
                }
            }
        }

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
    mProgram->release();

    // *** Draw cubes
    mFuncs->glBindVertexArray(mVAOCube);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    mProgram->bind();      
    {
        mFuncs->glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &pass1Index);

        mProgram->setUniformValue("Light.Position",  QVector4D(0.0f, 0.0f, 0.0f, 1.0f));
        mProgram->setUniformValue("Light.Intensity", QVector3D(0.9f, 0.9f, 0.9f));

        mProgram->setUniformValue("ViewNormalMatrix", ViewMatrix.normalMatrix());

        mProgram->setUniformValue("Material.Kd", 0.9f, 0.2f, 0.2f, 0.4f);
        mProgram->setUniformValue("Material.Ka", 0.0f, 0.0f, 0.0f, 0.0f);

        mProgram->setUniformValue("MaxNodes", maxNodes);

        float size = 2.0f;
        float pos  = 1.75f;

        ModelMatrixCube.setToIdentity();
        ModelMatrixCube.translate(-pos, -pos, pos);
        ModelMatrixCube.scale(size);

        QMatrix4x4 mv2 = ViewMatrix * ModelMatrixCube;
        mProgram->setUniformValue("ModelViewMatrix", mv2);
        mProgram->setUniformValue("NormalMatrix", mv2.normalMatrix());
        mProgram->setUniformValue("MVP", ProjectionMatrix * mv2);
        glDrawElements(GL_TRIANGLES, 6 * mCube->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        ModelMatrixCube.setToIdentity();
        ModelMatrixCube.translate(-pos, -pos, -pos);
        ModelMatrixCube.scale(size);

        mv2 = ViewMatrix * ModelMatrixCube;
        mProgram->setUniformValue("ModelViewMatrix", mv2);
        mProgram->setUniformValue("NormalMatrix", mv2.normalMatrix());
        mProgram->setUniformValue("MVP", ProjectionMatrix * mv2);
        glDrawElements(GL_TRIANGLES, 6 * mCube->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        ModelMatrixCube.setToIdentity();
        ModelMatrixCube.translate(-pos, pos, pos);
        ModelMatrixCube.scale(size);

        mv2 = ViewMatrix * ModelMatrixCube;
        mProgram->setUniformValue("ModelViewMatrix", mv2);
        mProgram->setUniformValue("NormalMatrix", mv2.normalMatrix());
        mProgram->setUniformValue("MVP", ProjectionMatrix * mv2);
        glDrawElements(GL_TRIANGLES, 6 * mCube->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        ModelMatrixCube.setToIdentity();
        ModelMatrixCube.translate(-pos, pos, -pos);
        ModelMatrixCube.scale(size);

        mv2 = ViewMatrix * ModelMatrixCube;
        mProgram->setUniformValue("ModelViewMatrix", mv2);
        mProgram->setUniformValue("NormalMatrix", mv2.normalMatrix());
        mProgram->setUniformValue("MVP", ProjectionMatrix * mv2);
        glDrawElements(GL_TRIANGLES, 6 * mCube->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        ModelMatrixCube.setToIdentity();
        ModelMatrixCube.translate(pos, pos, pos);
        ModelMatrixCube.scale(size);

        mv2 = ViewMatrix * ModelMatrixCube;
        mProgram->setUniformValue("ModelViewMatrix", mv2);
        mProgram->setUniformValue("NormalMatrix", mv2.normalMatrix());
        mProgram->setUniformValue("MVP", ProjectionMatrix * mv2);
        glDrawElements(GL_TRIANGLES, 6 * mCube->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        ModelMatrixCube.setToIdentity();
        ModelMatrixCube.translate(pos, pos, -pos);
        ModelMatrixCube.scale(size);

        mv2 = ViewMatrix * ModelMatrixCube;
        mProgram->setUniformValue("ModelViewMatrix", mv2);
        mProgram->setUniformValue("NormalMatrix", mv2.normalMatrix());
        mProgram->setUniformValue("MVP", ProjectionMatrix * mv2);
        glDrawElements(GL_TRIANGLES, 6 * mCube->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        ModelMatrixCube.setToIdentity();
        ModelMatrixCube.translate(pos, -pos, pos);
        ModelMatrixCube.scale(size);

        mv2 = ViewMatrix * ModelMatrixCube;
        mProgram->setUniformValue("ModelViewMatrix", mv2);
        mProgram->setUniformValue("NormalMatrix", mv2.normalMatrix());
        mProgram->setUniformValue("MVP", ProjectionMatrix * mv2);
        glDrawElements(GL_TRIANGLES, 6 * mCube->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        ModelMatrixCube.setToIdentity();
        ModelMatrixCube.translate(pos, -pos, -pos);
        ModelMatrixCube.scale(size);

        mv2 = ViewMatrix * ModelMatrixCube;
        mProgram->setUniformValue("ModelViewMatrix", mv2);
        mProgram->setUniformValue("NormalMatrix", mv2.normalMatrix());
        mProgram->setUniformValue("MVP", ProjectionMatrix * mv2);
        glDrawElements(GL_TRIANGLES, 6 * mCube->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }    
    mProgram->release();
}

void MyWindow::pass2()
{    
    mFuncs->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mFuncs->glBindVertexArray(mVAOFSQuad);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    mProgram->bind();
    {
        mFuncs->glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &pass2Index);

        QMatrix4x4 mv1 ,proj;

        mProgram->setUniformValue("ModelViewMatrix", mv1);
        mProgram->setUniformValue("NormalMatrix", mv1.normalMatrix());
        mProgram->setUniformValue("MVP", proj * mv1);

        // Render the full-screen quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
    }
    mProgram->release();
}

void MyWindow::initShaderStorage()
{
    glGenBuffers(2, buffers);
    maxNodes        = 20 * this->width() * this->height();
    GLint  nodeSize = 5 * sizeof(GLfloat) + sizeof(GLuint); // The size of a linked list node

    // Our atomic counter
    mFuncs->glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, buffers[COUNTER_BUFFER]);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);

    // The buffer for the head pointers, as an image texture
    glGenTextures(1, &headPtrTex);
    glBindTexture(GL_TEXTURE_2D, headPtrTex);
    mFuncs->glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, this->width(), this->height());
    mFuncs->glBindImageTexture(0, headPtrTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    // The buffer of linked lists
    mFuncs->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[LINKED_LIST_BUFFER]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, maxNodes * nodeSize, NULL, GL_DYNAMIC_DRAW);

    std::vector<GLuint> headPtrClearBuf(this->width() * this->height(), 0xffffffff);
    glGenBuffers(1, &clearBuf);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, clearBuf);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, headPtrClearBuf.size() * sizeof(GLuint), &headPtrClearBuf[0], GL_STATIC_COPY);
}

void MyWindow::clearBuffers() {
    GLuint zero = 0;

    mFuncs->glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, buffers[COUNTER_BUFFER] );
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &zero);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, clearBuf);
    glBindTexture(GL_TEXTURE_2D, headPtrTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->width(), this->height(), GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
}



void MyWindow::initShaders()
{
    QOpenGLShader vShader(QOpenGLShader::Vertex);
    QOpenGLShader fShader(QOpenGLShader::Fragment);    
    QFile         shaderFile;
    QByteArray    shaderSource;

    //Simple ADS
    shaderFile.setFileName(":/vshader.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "vertex compile: " << vShader.compileSourceCode(shaderSource);

    shaderFile.setFileName(":/fshader.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "frag   compile: " << fShader.compileSourceCode(shaderSource);

    mProgram = new (QOpenGLShaderProgram);
    mProgram->addShader(&vShader);
    mProgram->addShader(&fShader);
    qDebug() << "shader link: " << mProgram->link();
}

void MyWindow::PrepareTexture(GLenum TextureTarget, const QString& FileName, GLuint& TexObject, bool flip)
{
    QImage TexImg;

    if (!TexImg.load(FileName)) qDebug() << "Erreur chargement texture";
    if (flip==true) TexImg=TexImg.mirrored();

    glGenTextures(1, &TexObject);
    glBindTexture(TextureTarget, TexObject);
    glTexImage2D(TextureTarget, 0, GL_RGB, TexImg.width(), TexImg.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, TexImg.bits());
    glTexParameterf(TextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(TextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void MyWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    switch(keyEvent->key())
    {
        case Qt::Key_P:
            break;
        case Qt::Key_O:
            displayMode = !displayMode;
            break;
        case Qt::Key_Up:
            break;
        case Qt::Key_Down:
            break;
        case Qt::Key_Left:
            break;
        case Qt::Key_Right:
            break;
        case Qt::Key_Delete:
            break;
        case Qt::Key_PageDown:
            break;
        case Qt::Key_Home:
            break;
        case Qt::Key_Z:
            break;
        case Qt::Key_Q:
            break;
        case Qt::Key_S:
            break;
        case Qt::Key_D:
            break;
        case Qt::Key_A:
            break;
        case Qt::Key_E:
            break;
        default:
            break;
    }
}

void MyWindow::printMatrix(const QMatrix4x4& mat)
{
    const float *locMat = mat.transposed().constData();

    for (int i=0; i<4; i++)
    {
        qDebug() << locMat[i*4] << " " << locMat[i*4+1] << " " << locMat[i*4+2] << " " << locMat[i*4+3];
    }
}
