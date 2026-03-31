#include "executor.h"
#include "pager.h"
#include "btree.h"
#include "table.h"

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table){
    if (strcmp(input_buffer->buffer, ".exit") == 0){
        //this line flushes the data to file from the memory
        //this is required to save data to the file 
        //this ensures Atomicity (100% completion of any transaction)
        db_close(table);
        exit(EXIT_SUCCESS);
    }else if(strcmp(input_buffer->buffer, ".constants") == 0){
        printf("Constants=======\n");
        print_constants();
        return META_COMMAND_SUCCESS;
    }else if(strcmp(input_buffer->buffer, ".btree") == 0){
        printf("Tree=======\n");
        print_tree(table->pager, 0, 0);
        return META_COMMAND_SUCCESS;
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

ExecuteResult execute_insert(Statement* statement, Table* table){
    void* node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = (*leaf_node_num_cells(node));

    Row* row_to_insert = &(statement->row_to_insert);
    uint32_t key_to_insert = row_to_insert->id;
    Cursor* cursor = table_find(table, key_to_insert);
    if(cursor->cell_num < num_cells){
        uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
        if(key_at_index == key_to_insert){
            return EXECUTE_DUPLICATE_KEY;
        }
    }
    void* destination = cursor_value(cursor);
    if (destination == NULL){
        return EXECUTE_TABLE_FULL;
    }
    serialize_row(row_to_insert, destination);
    leaf_node_insert(cursor, row_to_insert->id, row_to_insert);
    free(cursor);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_delete(Statement* statement, Table* table) {
    void* node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = (*leaf_node_num_cells(node));

    if (num_cells == 0) {
        printf("Error: Cannot delete from an empty table.\n");
        return EXECUTE_SUCCESS;
    }

    uint32_t id_to_delete = statement->id_to_delete;
    Cursor* cursor = table_find(table, id_to_delete);
    if (cursor->cell_num < num_cells) {
        uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
        if (key_at_index != id_to_delete) {
            free(cursor);
            return EXECUTE_ID_NOT_FOUND;
        }
    }

    leaf_node_delete(cursor, id_to_delete);

    free(cursor);

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_update(Statement* statement, Table* table) {
    void* node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = (*leaf_node_num_cells(node));

    Row* row_to_insert = &(statement->row_to_insert);
    uint32_t key_to_update = row_to_insert->id;
    Cursor* cursor = table_find(table, key_to_update);
    if(cursor->cell_num < num_cells){
        uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
        if(key_at_index != key_to_update){
            free(cursor);
            return EXECUTE_ID_NOT_FOUND;
        }
    }

    void* destination = cursor_value(cursor);
    serialize_row(row_to_insert, destination);

    free(cursor);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table){
    Cursor* cursor = table_start(table);
    Row row;
    while(!(cursor->end_of_table)){
        deserialize_row(cursor_value(cursor), &row);
        print_row(&row);
        cursor_advance(cursor);
    }
    free(cursor);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table* table){
    switch (statement->type){
        case STATEMENT_INSERT:
            return execute_insert(statement, table);
        case STATEMENT_UPDATE:
            return execute_update(statement, table);
        case STATEMENT_DELETE:
            return execute_delete(statement, table);
        case STATEMENT_SELECT:
            return execute_select(statement, table);
    }
    return EXECUTE_SUCCESS; // unreachable, just to silence warnings
}
