#include "zip.h"

#include <QDebug>
#include <QDir>
#include <QUuid>
#include <cstdlib>

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

static void printErrNo() {
	if (errno) {
		qWarning() << "Error: " << strerror(errno);
	}
}

QMap<QString, QByteArray> LoadZip(QString filename) {
	QMap<QString, QByteArray> fileMap;

	mz_zip_archive zipFile;
	memset(&zipFile, 0, sizeof(zipFile));
	mz_bool status = mz_zip_reader_init_file(&zipFile, filename.toStdString().c_str(), 0);
	if (!status) {
		qWarning() << "Couldn't open " << filename << ". Reason: mz_zip_reader_init_file() failed!\n";
		printErrNo();
		mz_zip_reader_end(&zipFile);
		return {};
	}

	for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zipFile); i++) {
		mz_zip_archive_file_stat fileStat;
		if (!mz_zip_reader_file_stat(&zipFile, i, &fileStat)) {
			qWarning() << "Couldn't read " << filename << ". Reason: mz_zip_reader_file_stat() failed!\n";
			printErrNo();
			mz_zip_reader_end(&zipFile);
			return {};
		}

		/*
		qDebug() << "Filename: " << fileStat.m_filename << "\n"
			<< "Comment: " << fileStat.m_comment << "\n"
			<< "Uncompressed size: " << fileStat.m_uncomp_size << "\n"
			<< "Compressed size: " << fileStat.m_comp_size << "\n"
			<< "Is Dir: " << mz_zip_reader_is_file_a_directory(&zipFile, i) << "\n";
		*/
		size_t numBytes = (size_t) fileStat.m_uncomp_size;
		auto it = fileMap.insert(fileStat.m_filename, {});
		it->fill('\0', numBytes);
		// TODO: Speed up saving files by extracting directly to a file and caching it? mz_zip_reader_extract_file_to_file()
		mz_bool readStatus = mz_zip_reader_extract_file_to_mem(&zipFile, fileStat.m_filename, reinterpret_cast<void*>(it->data()), numBytes, 0);
		
		if (!readStatus) {
			qWarning() << "Couldn't read " << filename << ". Reason: mz_zip_reader_extract_file_to_mem() failed!\n";
			printErrNo();
			return {};
		}
	}

	mz_zip_reader_end(&zipFile);
	return fileMap;
}

QMap<QString, QString> LoadZipToFiles(QString filename) {
	const QDir tempDir{ QDir::tempPath() };

	auto newFileName = [tempDir](){		
		return tempDir.absoluteFilePath(QUuid::createUuid().toString());
	};

	QMap<QString, QString> fileMap;

	mz_zip_archive zipFile;
	memset(&zipFile, 0, sizeof(zipFile));
	mz_bool status = mz_zip_reader_init_file(&zipFile, filename.toStdString().c_str(), 0);
	if (!status) {
		qWarning() << "Couldn't open " << filename << ". Reason: mz_zip_reader_init_file() failed!\n";
		printErrNo();
		mz_zip_reader_end(&zipFile);
		return {};
	}

	for (int i = 0; i < (int)mz_zip_reader_get_num_files(&zipFile); i++) {
		mz_zip_archive_file_stat fileStat;
		if (!mz_zip_reader_file_stat(&zipFile, i, &fileStat)) {
			qWarning() << "Couldn't read " << filename << ". Reason: mz_zip_reader_file_stat() failed!\n";
			printErrNo();
			mz_zip_reader_end(&zipFile);
			return {};
		}

		auto fileName = newFileName();
		mz_bool readStatus = mz_zip_reader_extract_file_to_file(&zipFile, fileStat.m_filename, fileName.toStdString().c_str(), 0);
		if (!readStatus) {
			qWarning() << "Couldn't read " << filename << ". Reason: mz_zip_reader_extract_file_to_mem() failed!\n";
			printErrNo();
			return {};
		}

		fileMap.insert(fileStat.m_filename, fileName);
	}

	mz_zip_reader_end(&zipFile);
	return fileMap;
}

bool WriteZip(QString filename, const QMap<QString, QString>& filenames) {
	// Open zip and dump into directory
	mz_zip_archive zipArchive;
	memset(&zipArchive, 0, sizeof(zipArchive));
	mz_bool status = mz_zip_writer_init_file(&zipArchive, filename.toStdString().c_str(), 0);
	if (!status) {
		qWarning() << "Couldn't open " << filename << ". Reason: mz_zip_writer_init_file() failed!";
		printErrNo();
		mz_zip_writer_end(&zipArchive);
		return false;
	}

	for (auto it = filenames.begin(); it != filenames.end(); ++it) {
		QChar c;
		mz_bool writeStatus = mz_zip_writer_add_file(&zipArchive, it.key().toStdString().c_str(), it.value().toStdString().c_str(), nullptr, 0, MzNoCompression);
		if (!writeStatus) {
			qWarning() << "Couldn't write " << it.value() << ". Reason: mz_zip_writer_add_file() failed!";
			printErrNo();
			mz_zip_writer_end(&zipArchive);
			return false;
		}
	}

	mz_zip_writer_finalize_archive(&zipArchive);
	mz_zip_writer_end(&zipArchive);
	return true;
}