/**
 * ExperimentPackager.cpp
 *
 * History:
 * paul on 1/25/06 - Created.
 *
 * Copyright 2006 MIT. All rights reserved.
 */


#include "ExperimentPackager.h"
#include "Utilities.h"
#include "LoadingUtilities.h"
#include "SystemEventFactory.h"
#include <iostream>
#include <fstream>

#include "PlatformDependentServices.h"
#include "boost/filesystem/path.hpp"
#include "boost/algorithm/string/replace.hpp"


BEGIN_NAMESPACE_MW


Datum ExperimentPackager::packageSingleFile(const Datum &contents, const std::string &filename) {
    Datum unit(M_DICTIONARY, M_EXPERIMENT_PACKAGE_NUMBER_ELEMENTS_PER_UNIT);
    
    Datum name;
	name.setString(filename.c_str(), filename.length()+1);
	unit.addElement(M_PACKAGER_FILENAME_STRING, name);
    
	unit.addElement(M_PACKAGER_CONTENTS_STRING, contents);
    
	return unit;
}


Datum
ExperimentPackager::packageSingleFile(const boost::filesystem::path filepath, const std::string filename) {
	namespace bf = boost::filesystem;
    
	std::ifstream mediaFile;
	mediaFile.open(filepath.string().c_str(), std::ios::binary);
	
	// get length of file:
	mediaFile.seekg(0, std::ios::end);
	int length = mediaFile.tellg();
	// if the file was never opened
	if(length <= 0) {
		mediaFile.close();
        Datum undef;
		return undef;
	}
	
	char * buffer = new char[length];
	
	mediaFile.seekg(0, std::ios::beg);
	mediaFile.read(buffer, length);
	mediaFile.close();
	
    Datum bufferData;
	bufferData.setString(buffer, length);
	
	delete [] buffer;
	
	return packageSingleFile(bufferData, filename);
}

Datum ExperimentPackager::packageExperiment(const boost::filesystem::path filename) {
	namespace bf = boost::filesystem;
	IncludedFilesParser parser(filename.string());
	std::string working_path_string;
    Datum include_files;
	
	try{
		parser.parse(false);
		working_path_string = parser.getWorkingPathString();
		include_files = parser.getIncludedFilesManifest();
	} catch(std::exception& e){
		merror(M_PARSER_MESSAGE_DOMAIN, "Experiment packaging failed: %s",
										e.what());
		return Datum();
	}
	
    Datum eventPayload(M_DICTIONARY, M_EXPERIMENT_PACKAGE_NUMBER_ELEMENTS);
    
    {
        // Use getDocumentData to get the experiment file with any preprocessing and/or
        // XInclude substitutions already applied
        std::vector<xmlChar> fileData;
        parser.getDocumentData(fileData);
        
        Datum contents(reinterpret_cast<char *>(&(fileData.front())), fileData.size());
        eventPayload.addElement(M_PACKAGER_EXPERIMENT_STRING,
                                packageSingleFile(contents, XMLParser::squashFileName(filename.string())));
    }
	
	if(include_files.getNElements() >= 1) {
        Datum mediaFilesPayload(M_LIST, include_files.getNElements());
		
		
		for(int i=0; i< include_files.getNElements();  ++i) {
			
			// DDC there seem to be a lot of unnecessary steps here
			// simplified hackily for the moment
			std::string mediaName(include_files.getElement(i).getString());
			
			bf::path mediaPath = expandPath(working_path_string, mediaName);
			
			//bf::path mediaPath(include_files.getElement(i).getElement(M_PACKAGER_FULL_NAME).getString());
			//std::string mediaName(include_files.getElement(i).getElement(M_PACKAGER_RELATIVE_NAME).getString());
            Datum mediaElement = packageSingleFile(mediaPath, mediaName);
			
			if(!mediaElement.isUndefined()) {
				mediaFilesPayload.addElement(mediaElement);
			} else {
				merror(M_FILE_MESSAGE_DOMAIN, 
					   "Can't find file: %s", mediaPath.string().c_str());
                Datum undef;
				return undef;
			}
		}
		
		eventPayload.addElement(M_PACKAGER_MEDIA_BUFFERS_STRING, 
								mediaFilesPayload);
    }
	
	return SystemEventFactory::systemEventPackage(M_SYSTEM_DATA_PACKAGE, 
											 M_EXPERIMENT_PACKAGE, 
											 eventPayload);
}


IncludedFilesParser::IncludedFilesParser(const std::string &_path) :
    XMLParser(_path, "MWMediaPackagerTransformation.xsl"),
    manifest(M_LIST, 1)
{ }


void IncludedFilesParser::_processCreateDirective(xmlNode *node) {
    xmlNode *child = node->children;
    
    while (child != NULL) {
        string name((const char *)(child->name));
        
        if (name == "path") {

            string filePath = _getContent(child);
            manifest.addElement(filePath);

        } else if (name == "directory_path") {

            string directoryPath = _getContent(child);
            std::vector<std::string> filePaths;

            mw::getFilePaths(getWorkingPathString(), directoryPath, filePaths);
            for (std::vector<std::string>::iterator iter = filePaths.begin(); iter != filePaths.end(); iter++) {
                manifest.addElement(*iter);
            }
            
        }
        
        child = child->next;
    }
}


void IncludedFilesParser::_processAnonymousCreateDirective(xmlNode *node) {
    _processCreateDirective(node);
}


END_NAMESPACE_MW























