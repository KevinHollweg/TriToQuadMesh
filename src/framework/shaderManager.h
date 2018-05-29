#pragma once

#include "config.h"

#include <sys/stat.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif




#define HG_SHADER_VERSION "#version 430\n"
#define HG_NUM_PROGRAM_TYPES 6

#ifndef _WIN32
struct ULARGE_INTEGER {
	long QuadPart;
};
#endif



namespace SHADERTYPE_FLAG {
	const size_t NONE = 0;
	const size_t VERTEX = 1 << 0;
	const size_t TESSCONTROL = 1 << 1;
	const size_t TESSEVALUATION = 1 << 2;
	const size_t GEOMETRY = 1 << 3;
	const size_t FRAGMENT = 1 << 4;
	const size_t COMPUTE = 1 << 5;
};

namespace PRGSTYLE {
	const int VERTEX_FRAGMENT =
		SHADERTYPE_FLAG::VERTEX |
		SHADERTYPE_FLAG::FRAGMENT;

	const int VERTEX_GEOMETRY_FRAGMENT =
		SHADERTYPE_FLAG::VERTEX |
		SHADERTYPE_FLAG::GEOMETRY |
		SHADERTYPE_FLAG::FRAGMENT;

	const int VERTEX_TESSCONTROL_TESSEVALUATION_FRAGMENT =
		SHADERTYPE_FLAG::VERTEX |
		SHADERTYPE_FLAG::TESSCONTROL |
		SHADERTYPE_FLAG::TESSEVALUATION |
		SHADERTYPE_FLAG::FRAGMENT;

	const int VERTEX_TESSCONTROL_TESSEVALUATION_GEOMETRY_FRAGMENT =
		SHADERTYPE_FLAG::VERTEX |
		SHADERTYPE_FLAG::TESSCONTROL |
		SHADERTYPE_FLAG::TESSEVALUATION |
		SHADERTYPE_FLAG::GEOMETRY |
		SHADERTYPE_FLAG::FRAGMENT;

	const int COMPUTE = SHADERTYPE_FLAG::COMPUTE;
}




#define HG_SHADER_VERTEX_STR					"--vertex"
#define HG_SHADER_TESSELATION_CONTROL_STR		"--tessControl"
#define HG_SHADER_TESSELATION_EVALUATION_STR	"--tessEval"
#define HG_SHADER_GEOMETRY_STR					"--geometry"
#define HG_SHADER_FRAGMENT_STR					"--fragment"
#define HG_SHADER_COMPUTE_STR					"--compute"

class FileDependency {
public:
	FileDependency(const std::string& filename);
	
	bool isFileChanged();
	const std::string& getFileName() { return m_fileName; };

private:
	std::string m_fileName;
    ULARGE_INTEGER m_writeTime;

};

class Program {
public:
	Program(const std::string& fileName, size_t programStageTypeFlags);
	~Program();

	bool isSourceChanged();
	bool loadProgramAndCompile(const std::string& directoryPrefix = "");
	void deleteProgramGL();

	GLuint getProgramGL();

private:


	FileDependency m_mainFile;
	std::vector<FileDependency> m_includedFiles;

	size_t m_programStageTypeFlags;
	GLint m_compileStatus;
	GLuint m_program;
	GLuint m_shaders[HG_NUM_PROGRAM_TYPES];
};

class ShaderManager {
public:
	ShaderManager(const std::string& directoryPrefix, const std::string& fileExtension, bool autoReload = true);
	~ShaderManager();

	void registerProgram(const std::string& identifier, size_t programStageTypeFlags);
	 GLuint getProgramGL(const std::string& identifier);

	void update();

protected:
	std::map<std::string, int> m_nameToProgramID;
	std::vector<Program*> m_programs;
	std::string m_directoryPrefix;
	std::string m_fileExtension;
	bool m_firstRun, m_autoReload;

};
