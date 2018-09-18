#include "zip.h"

#include <QDebug>

#if defined(__GNUC__) && !defined(__APPLE__)
// Ensure we get the 64-bit variants of the CRT's file I/O calls
// (from miniz example2.c)
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1
#endif
#endif

#include "miniz.c"  // NB: Intentionally including miniz.c

QMap<QString, QByteArray> LoadZip(QString filename) {
	QMap<QString, QByteArray> fileMap;

	mz_zip_archive zipFile;
	memset(&zipFile, 0, sizeof(zipFile));
	mz_bool status = mz_zip_reader_init_file(&zipFile, filename.toStdString().c_str(), 0);
	if (!status) {
		qWarning() << "Couldn't open " << filename << ". Reason: mz_zip_reader_init_file() failed!\n";
		mz_zip_reader_end(&zipFile);
		return {};
	}

	for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zipFile); i++) {
		mz_zip_archive_file_stat fileStat;
		if (!mz_zip_reader_file_stat(&zipFile, i, &fileStat)) {
			qWarning() << "Couldn't read " << filename << ". Reason: mz_zip_reader_file_stat() failed!\n";
			mz_zip_reader_end(&zipFile);
			return {};
		}

		size_t numBytes = (size_t)fileStat.m_uncomp_size;
		auto it = fileMap.insert(fileStat.m_filename, {});
		it->fill('\0', numBytes);
		mz_bool readStatus = mz_zip_reader_extract_file_to_mem(&zipFile, fileStat.m_filename, reinterpret_cast<void*>(it->data()), numBytes, 0);
		
		if (!readStatus) {
			qWarning() << "Couldn't read " << filename << ". Reason: mz_zip_reader_extract_file_to_file() failed!\n";
			return {};
		}
	}

	mz_zip_reader_end(&zipFile);
	return fileMap;
}
