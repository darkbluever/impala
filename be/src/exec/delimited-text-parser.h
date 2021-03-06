// Copyright (c) 2012 Cloudera, Inc. All rights reserved.

#ifndef IMPALA_EXEC_DELIMITED_TEXT_PARSER_H
#define IMPALA_EXEC_DELIMITED_TEXT_PARSER_H

#include "exec/hdfs-scanner.h"
#include "exec/hdfs-scan-node.h"
#include "util/sse-util.h"

namespace impala {

class DelimitedTextParser {
 public:

  // The Delimited Text Parser parses text rows that are delimited by specific
  // characters:
  //   tuple_delim: delimits tuples
  //   field_delim: delimits fields
  //   collection_item_delim: delimits collection items
  //   escape_char: escape delimiters, make them part of the data.
  // Other parameters to the creator:
  //   map_column_to_slot: maps a column in the input to the output slot.
  //   start_column: the index in the above vector where the columns start.
  //                 it will be non-zero if there are partition columns.
  //   timer: timer to use to time the parsing operation, or NULL.
  //
  // The main method is ParseData which fills in a vector of
  // pointers and lengths to the fields.  It also can handle an excape character
  // which masks a tuple or field delimiter that occurs in the data.
  DelimitedTextParser(HdfsScanNode* scan_node,
                      char tuple_delim, char field_delim_ = '\0',
                      char collection_item_delim = '\0', char escape_char = '\0');

  ~DelimitedTextParser();

  // Called to initialize parser at beginning of scan range.
  void ParserReset();

  // Check if we are at the start of a tuple.
  bool AtTupleStart() { return column_idx_ == scan_node_->num_partition_keys(); }

  char escape_char() const { return escape_char_; }

  // Parses a byte buffer for the field and tuple breaks.
  // This function will write the field start & len to field_locations
  // which can then be written out to tuples.
  // This function uses SSE ("Intel x86 instruction set extension
  // 'Streaming Simd Extension') if the hardware supports SSE4.2
  // instructions.  SSE4.2 added string processing instructions that
  // allow for processing 16 characters at a time.  Otherwise, this
  // function walks the file_buffer_ character by character.
  // Input Parameters:
  //   max_tuples: The maximum number of tuples that should be parsed.
  //               This is used to control how the batching works.
  //   remaining_len: Length of data remaining in the byte_buffer_pointer.
  //   byte_buffer_pointer: Pointer to the buffer containing the data to be parsed.
  // Output Parameters:
  //   field_locations: array of pointers to data fields and their lengths
  //   num_tuples: Number of tuples parsed
  //   num_fields: Number of materialized fields parsed
  //   next_column_start: pointer within file_buffer_ where the next field starts
  //                      after the return from the call to ParseData
  Status ParseFieldLocations(int max_tuples, int64_t remaining_len,
      char** byte_buffer_ptr, char** row_end_locations,
      FieldLocation* field_locations,
      int* num_tuples, int* num_fields, char** next_column_start);

  // Parse a single tuple from buffer.  
  // - buffer/len are input parameters for the entire record.
  // - on return field_locations will contain the start/len for each materialized
  //   col.
  // - *num_fields returns the number of fields processed.
  // This function is used to parse sequence file records which do not need to
  // parse for tuple delimiters.
  template <bool process_escapes>
  void ParseSingleTuple(int64_t len, char* buffer, FieldLocation* field_locations, 
      int* num_fields);

  // FindFirstInstance returns the position after the first non-escaped tuple
  // delimiter from the starting offset.
  // Used to find the start of a tuple if jumping into the middle of a text file.
  // Also used to find the sync marker for Sequenced and RC files.
  // If no tuple delimiter is found within the buffer, return -1;
  int FindFirstInstance(const char* buffer, int len);

  // Find a sync block if jumping into the middle of a Sequence or RC file.
  // The sync block is always proceeded by an indicator of 4 bytes of -1 (0xff).
  // This will move the Bytestream to the beginning of the sync indicator (the -1).
  // If no sync block is found we will be at or beyond the end of the
  // scan range.
  // Inputs:
  //   end_of_range: the end of the scan range we are searching.
  //   sync_size: the size of the sync block.
  //   sync: the sync bytes to look for.
  //   byte_stream: the byte stream to read.
  Status FindSyncBlock(int end_of_range, int sync_size, uint8_t*
                       sync, ByteStream*  byte_stream);

  // Will we return the current column to the query?
  // Hive allows cols at the end of the table that are not in the schema.  We'll
  // just ignore those columns
  bool ReturnCurrentColumn() const {
    return column_idx_ < num_cols_ && is_materialized_col_[column_idx_];
  }

  // Fill in columns missing at the end of the tuple.
  // len and last_column may contain the length and the pointer to the
  // last column on which the file ended without a delimiter.
  // Fills in the offsets and lengths in field_locations.
  // If parsing stopped on a delimiter and there is no last column then len will be  0.
  // Other columns beyond that are filled with 0 length fields.
  // num_fields points to an initialized count of fields and will incremented
  // by the number fields added.
  // field_locations will be updated with the start and length of the fields.
  template <bool process_escapes>
  void FillColumns(int len, char** last_column, 
                   int* num_fields, impala::FieldLocation* field_locations);

 private:
  // Initialize the parser state.
  void ParserInit(HdfsScanNode* scan_node);

  // Helper routine to add a column to the field_locations vector.
  // Template parameter:
  //   process_escapes -- if true the the column may have escape characters
  //                      and the negative of the len will be stored.
  //   len: lenght of the current column.
  // Input/Output:
  //   next_column_start: Start of the current column, moved to the start of the next.
  //   num_fields: current number of fields processed, updated to next field.
  // Output:
  //   field_locations: updated with start and length of current field.
  template <bool process_escapes>
  void AddColumn(int len, char** next_column_start, int* num_fields,
                 FieldLocation* field_locations);

  // Helper routine to parse delimited text using SSE instructions.  
  // Identical arguments as ParseFieldLocations. 
  // If the template argument, 'process_escapes' is true, this function will handle
  // escapes, otherwise, it will assume the text is unescaped.  By using templates,
  // we can special case the un-escaped path for better performance.  The unescaped
  // path is optimized away by the compiler.
  template <bool process_escapes>
  void ParseSse(int max_tuples, int64_t* remaining_len,
      char** byte_buffer_ptr, char** row_end_locations_,
      FieldLocation* field_locations,
      int* num_tuples, int* num_fields, char** next_column_start);

  // ScanNode reference to map columns in the data to slots in the tuples.
  HdfsScanNode* scan_node_;

  // SSE(xmm) register containing the tuple search character.
  __m128i xmm_tuple_search_;

  // SSE(xmm) register containing the delimiter search character.
  __m128i xmm_delim_search_;

  // SSE(xmm) register containing the escape search character.
  __m128i xmm_escape_search_;

  // Character delimiting fields (to become slots).
  char field_delim_;

  // Escape character.
  char escape_char_;

  // Character delimiting collection items (to become slots).
  char collection_item_delim_;

  // Character delimiting tuples.
  char tuple_delim_;

  // Whether or not the current column has an escape character in it
  // (and needs to be unescaped)
  bool current_column_has_escape_;

  // Whether or not the previous character was the escape character
  bool last_char_is_escape_;

  // Used for special processing of \r.  
  // This will be the offset of the last instance of \r from the end of the
  // current buffer being searched unless the last row delimiter was not a \r in which
  // case it will be -1.  If the last character in a buffer is \r then the value
  // will be 0.  At the start of processing a new buffer if last_row_delim_offset_ is 0
  // then it is set to be one more than the size of the buffer so that if the buffer
  // starts with \n it is processed as \r\n.
  int32_t last_row_delim_offset_;
  
  // Precomputed masks to process escape characters
  uint16_t low_mask_[16];
  uint16_t high_mask_[16];

  // Number of non partition cols in the table.  Replicated from ScanNode for 
  // performance reasons.
  int num_cols_;

  // For each col index [0, num_cols), true if the column should be materialized.
  // Replicated from the ScanNode for performance reasons.  Memory owned by this
  // object.
  bool* is_materialized_col_;

  // Index to keep track of the current current column in the current file
  int column_idx_;
};

}// namespace impala
#endif// IMPALA_EXEC_DELIMITED_TEXT_PARSER_H
