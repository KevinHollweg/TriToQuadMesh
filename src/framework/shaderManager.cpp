#include "shaderManager.h"


static const char* stageTypeUppercaseNames[] = {
	"VERTEX",
	"TESSELATION_CONTROL",
	"TESSELATION_EVALUATION",
	"GEOMETRY",
	"FRAGMENT",
	"COMPUTE"
};

static const char* stageTypeIdentifierNames[] = {
	HG_SHADER_VERTEX_STR,
	HG_SHADER_TESSELATION_CONTROL_STR,
	HG_SHADER_TESSELATION_EVALUATION_STR,
	HG_SHADER_GEOMETRY_STR,
	HG_SHADER_FRAGMENT_STR,
	HG_SHADER_COMPUTE_STR,
};

static const int stageTypeFlags[] = {
	SHADERTYPE_FLAG::VERTEX,
	SHADERTYPE_FLAG::TESSCONTROL,
	SHADERTYPE_FLAG::TESSEVALUATION,
	SHADERTYPE_FLAG::GEOMETRY,
	SHADERTYPE_FLAG::FRAGMENT,
	SHADERTYPE_FLAG::COMPUTE
};


static const int stageGLShaderType[] = {
	GL_VERTEX_SHADER,
	GL_TESS_CONTROL_SHADER,
	GL_TESS_EVALUATION_SHADER,
	GL_GEOMETRY_SHADER,
	GL_FRAGMENT_SHADER,
	GL_COMPUTE_SHADER
};

bool startsWith(const std::string& str, const std::string& substr) {
	return substr.length() <= str.length() 
		&& equal(substr.begin(), substr.end(), str.begin());
}

static ULARGE_INTEGER getFileWriteTime(const char* name) {
#ifndef __WIN32
	struct stat sb;
	stat(name, &sb);
	ULARGE_INTEGER write;
	write.QuadPart = sb.st_mtime;
	return write;
#else
	ULARGE_INTEGER create, access, write;
	HANDLE hFile = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	write.QuadPart = 0;
	if(hFile !=  INVALID_HANDLE_VALUE) { 
		GetFileTime(hFile, LPFILETIME(&create), LPFILETIME(&access), LPFILETIME(&write));
	}
	CloseHandle(hFile);
	return write;
#endif
}

std::string uintToString(size_t i) {
   std::stringstream ss;
   ss << i;
   return ss.str();
}

struct SourceLine {
	SourceLine(size_t l, const std::string& s) : lineNumber(l), string(s) {}
	size_t lineNumber;
	std::string string;
};

static bool readLinesFromFile(const char* filename, std::vector<SourceLine>& lines) {
	lines.clear();
	std::ifstream ifs;
	ifs.open(filename, std::ios::in);
	if (!ifs.good()) {
		std::cerr << "readLinesFromFile() - could not open file " << filename << std::endl;
		return false;
	}

	std::string str;
	size_t lineNumber = 0;
	while (ifs.good() && getline(ifs, str)) {
		lines.push_back(SourceLine(++lineNumber, str));
		//std::cerr << "s" << str << std::endl;
	}
	ifs.close();
	return true;
}

static void copySourceLinesIntoString(std::string& dst, const std::vector<SourceLine>& lines) {
	dst.clear();
	for (auto it = lines.begin(); it != lines.end(); ++it) {
		dst += it->string + "\n";
	}
}


FileDependency::FileDependency(const std::string& filename) : m_fileName(filename) {
	m_writeTime = getFileWriteTime(m_fileName.c_str());
}


bool FileDependency::isFileChanged() {
	ULARGE_INTEGER currentWriteTime = getFileWriteTime(m_fileName.c_str());
	if (m_writeTime.QuadPart < currentWriteTime.QuadPart) {
		m_writeTime = currentWriteTime;
		return true;
	}
	return false;
}


Program::Program(const std::string& fileName, size_t programStageTypeFlags) : m_mainFile(fileName), m_programStageTypeFlags(programStageTypeFlags), m_compileStatus(GL_FALSE), m_program(0) {}
Program::~Program() { deleteProgramGL(); }

bool Program::isSourceChanged() {
	// Check main file for changes
	if (m_mainFile.isFileChanged())
		return true;

	// Check includes for changes
	for (auto it = m_includedFiles.begin(); it != m_includedFiles.end(); ++it) {
		if (it->isFileChanged())
			return true;
	}

	// No changes
	return false;
}

bool Program::loadProgramAndCompile(const std::string& directoryPrefix /* = "" */) {
	deleteProgramGL();

	m_compileStatus = GL_FALSE;
	size_t programStageTypeFlags = 0;
	std::vector<SourceLine> programLines;
	if (!readLinesFromFile(m_mainFile.getFileName().c_str(), programLines)) {
		return false;
	}

	size_t stageLineIndices[HG_NUM_PROGRAM_TYPES];
	// Initialize the stage indices
	for (size_t i = 0; i < HG_NUM_PROGRAM_TYPES; ++i) {
		stageLineIndices[i] = -1;
	}

	// Find the stage identifiers "--vertex", "--fragment" etc.
	// and find the line index for the stage in the main shader file
	size_t mainFileLineIndex = 0;
	for (auto it = programLines.begin(); it != programLines.end(); ++it) {
		for (size_t i = 0; i < HG_NUM_PROGRAM_TYPES; ++i) {
			if (startsWith(it->string, stageTypeIdentifierNames[i])) {
				stageLineIndices[i] = mainFileLineIndex;
				programStageTypeFlags |= stageTypeFlags[i];
			}
		}
		mainFileLineIndex++;
	}

	// Verify the found program stages
	if (programStageTypeFlags != m_programStageTypeFlags) {
		std::cerr << "loadProgramAndCompile() - \"" << m_mainFile.getFileName() << "\" - programStageTypeFlags verification failed." << std::endl;
        std::cerr << programStageTypeFlags << " != " << m_programStageTypeFlags << std::endl;
		return false;
	}

	// Parse and handle includes.
	// Only one inclusion depth level supported.
	const std::string includeBeginStr("#include \"");
	const std::string includeEndStr("\"");
	m_includedFiles.clear();

	for (auto it = programLines.begin(); it != programLines.end(); ++it) {
		// No include found in this line, continue
		if (!startsWith(it->string, includeBeginStr))
			continue;

		const size_t positionIncludeBegin = includeBeginStr.size();

		size_t positionIncludeEnd = it->string.find(includeEndStr, positionIncludeBegin);
		if (positionIncludeEnd == std::string::npos)
			continue;

		std::string includeFileName = directoryPrefix + it->string.substr(positionIncludeBegin, positionIncludeEnd - positionIncludeBegin);

		std::vector<SourceLine> includeLines;
		if (!readLinesFromFile(includeFileName.c_str(), includeLines))
			return false;

		//if (!includeLines.empty()) {
			// Replace the old line with the code from the included file
			// and handle line numbers
			copySourceLinesIntoString(it->string, includeLines);
			it->string =
				"#line 1 " + uintToString(1000+m_includedFiles.size()) + " \n" + it->string +
				"#line " + uintToString(it->lineNumber) + " 0\n";
		//}

		m_includedFiles.push_back(FileDependency(includeFileName));
	}

	// After parsing all includes, copy the shader stages together into one single string
	// and replace the stage identifiers "--vertex", "--fragment" etc.
	size_t stageOffsets[HG_NUM_PROGRAM_TYPES];
	std::string concatinatedSource;
	mainFileLineIndex = 0;
	for (auto it = programLines.begin(); it != programLines.end(); ++it) {
		std::string append = it->string + "\n";
		for (size_t i = 0; i < HG_NUM_PROGRAM_TYPES; ++i) {
			if (stageLineIndices[i] == mainFileLineIndex) {
				stageOffsets[i] = concatinatedSource.size() + 1;
				// The X will be replaced with a '\0' in the final string
				append = "X#line " + uintToString(it->lineNumber + 1) + "\n";
			}
		}
		concatinatedSource += append;
		mainFileLineIndex++;
	}

	// Copy the concatinated source into a c-string and insert '\0''s
	char* concatinatedSourceCStr = new char[concatinatedSource.size()+1];
	char* shaderSources[HG_NUM_PROGRAM_TYPES];
	memcpy(concatinatedSourceCStr, concatinatedSource.c_str(), concatinatedSource.size());
	concatinatedSourceCStr[concatinatedSource.size()] = '\0';
	for (size_t i = 0; i < HG_NUM_PROGRAM_TYPES; ++i) {
		if (stageLineIndices[i] != (size_t)-1) {
			concatinatedSourceCStr[stageOffsets[i] - 1] = '\0';
			shaderSources[i] = concatinatedSourceCStr + stageOffsets[i];
		} else {
			shaderSources[i] = 0;
		}
	}

	// Compile shaders
	char buf[1024];
	GLint infoLogLength;

	for (size_t i = 0; i < HG_NUM_PROGRAM_TYPES; ++i) {
		if (shaderSources[i] == 0) {
			m_shaders[i] = 0;
			continue;
		}
		m_shaders[i] = glCreateShader(stageGLShaderType[i]);

		const char* stitchedSources[] = {HG_SHADER_VERSION, shaderSources[i], NULL};
		// Debug output the concatinated source
		//std::cerr << stitchedSources[0] << stitchedSources[1] << std::endl;


		glShaderSource(m_shaders[i], 2, &stitchedSources[0], 0);
		glCompileShader(m_shaders[i]);
		glGetShaderiv(m_shaders[i], GL_COMPILE_STATUS, &m_compileStatus);
		glGetShaderiv(m_shaders[i], GL_INFO_LOG_LENGTH, &infoLogLength);
		glGetShaderInfoLog(m_shaders[i], 1024, 0, buf);

		if (infoLogLength > 1) {
			std::cerr << stageTypeUppercaseNames[i] << " [" << m_mainFile.getFileName() << "]:" << std::endl << buf << std::endl << "--------" << std::endl;
		}

		if (m_compileStatus != GL_TRUE) {
			std::cerr << stageTypeUppercaseNames[i] << " [" << m_mainFile.getFileName() << "]: " << std::endl << " COMPILE FAILED" << std::endl;
			delete [] concatinatedSourceCStr;
			return false;
		}
	}

	delete [] concatinatedSourceCStr;

	// Create program and attach shaders
	m_program = glCreateProgram();
	for (int i = 0; i < HG_NUM_PROGRAM_TYPES; ++i) {
		if (shaderSources[i] == 0)
			continue;
		glAttachShader(m_program, m_shaders[i]);
	}

	// Link the program
	glLinkProgram(m_program);
	glGetProgramiv(m_program, GL_LINK_STATUS, &m_compileStatus);
	glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &infoLogLength);
	glGetProgramInfoLog(m_program, 1024, 0, buf);

	if (infoLogLength > 1) {
        std::cout << "LINK [" << m_mainFile.getFileName() << "]:" << std::endl << buf << std::endl << "--------" << std::endl;
	}

	if (m_compileStatus != GL_TRUE) {
        std::cout << "LINK [" << m_mainFile.getFileName() << "]: FAILED" << std::endl;
		return false;
	} else {
        std::cout << "LINK [" << m_mainFile.getFileName() << "]: SUCCESS" << std::endl;
	}

	return true;


}

void Program::deleteProgramGL() {
	if (m_program == 0)
		return;
	for (int i = 0; i < HG_NUM_PROGRAM_TYPES; ++i) {
		if (m_shaders[i]) {
			glDeleteShader(m_shaders[i]);
			m_shaders[i] = 0;
		}
	}

	glDeleteProgram(m_program);
	m_program = 0;
}


GLuint Program::getProgramGL() {
	return m_program;
}


ShaderManager::ShaderManager(const std::string& directoryPrefix, const std::string& fileExtension, bool autoReload /* = true */) : m_directoryPrefix(directoryPrefix), m_fileExtension(fileExtension), m_firstRun(true), m_autoReload(autoReload) {}

ShaderManager::~ShaderManager() {
	m_programs.clear();
}

void ShaderManager::registerProgram(const std::string& identifier, size_t programStageTypeFlags ) {
	m_nameToProgramID[identifier] = (int)m_programs.size();
	m_programs.push_back(new Program(m_directoryPrefix + identifier + m_fileExtension, programStageTypeFlags));
}

	GLuint ShaderManager::getProgramGL(const std::string& identifier) {
	auto it = m_nameToProgramID.find(identifier);
	if (it == m_nameToProgramID.end())
		return -1;

	return m_programs[it->second]->getProgramGL();
}

void ShaderManager::update() {
	for (auto it = m_programs.begin(); it != m_programs.end(); ++it) {
		Program& p = **it;
		if ((p.isSourceChanged() && m_autoReload) || m_firstRun) {
#ifndef _WIN32
			usleep(10);
#else
			Sleep(10);
#endif
			p.deleteProgramGL();
			p.loadProgramAndCompile(m_directoryPrefix);
		}
	}
	m_firstRun = false;
}

