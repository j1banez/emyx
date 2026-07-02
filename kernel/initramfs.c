#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/boot.h>
#include <kernel/endian.h>
#include <kernel/initramfs.h>
#include <kernel/printk.h>

static int initramfs_header(const uint8_t **bytes, uint32_t *archive_size,
    uint32_t *file_count)
{
    const uint8_t *archive;
    uint32_t size;
    uint32_t count;

    archive = (const uint8_t *)boot_module_start();
    size = boot_module_size();

    if (archive == NULL || size < EMXA_HEADER_SIZE)
        return -1;
    if (archive[0] != EMXA_MAGIC0 || archive[1] != EMXA_MAGIC1 ||
            archive[2] != EMXA_MAGIC2 || archive[3] != EMXA_MAGIC3)
        return -1;

    count = read_le32(archive + 4);
    if (count > (size - EMXA_HEADER_SIZE) / EMXA_ENTRY_SIZE)
        return -1;

    *bytes = archive;
    *archive_size = size;
    *file_count = count;
    return 0;
}

static int path_matches(const uint8_t *entry_path, const char *path)
{
    uint32_t i;

    for (i = 0; i < EMXA_PATH_SIZE; i++) {
        if (entry_path[i] != (uint8_t)path[i])
            return 0;
        if (path[i] == '\0')
            return 1;
    }

    return 0;
}

int initramfs_find(const char *path, const void **data, uint32_t *size)
{
    const uint8_t *archive;
    const uint8_t *entry;
    uint32_t archive_size;
    uint32_t file_count;
    uint32_t i;
    uint32_t offset;
    uint32_t file_size;

    if (path == NULL || data == NULL || size == NULL)
        return -1;
    if (initramfs_header(&archive, &archive_size, &file_count) != 0)
        return -1;

    for (i = 0; i < file_count; i++) {
        entry = archive + EMXA_HEADER_SIZE + i * EMXA_ENTRY_SIZE;
        if (!path_matches(entry, path))
            continue;

        offset = read_le32(entry + EMXA_PATH_SIZE);
        file_size = read_le32(entry + EMXA_PATH_SIZE + 4);
        if (offset > archive_size || file_size > archive_size - offset)
            return -1;

        *data = archive + offset;
        *size = file_size;
        return 0;
    }

    return -1;
}

void initramfs_list(void)
{
    const uint8_t *archive;
    const uint8_t *entry;
    uint32_t archive_size;
    uint32_t file_count;
    uint32_t i;
    uint32_t offset;
    uint32_t file_size;
    uint32_t j;

    if (initramfs_header(&archive, &archive_size, &file_count) != 0) {
        printk("initramfs: invalid module\n");
        return;
    }

    printk("initramfs: files=%x size=%x\n", file_count, archive_size);
    for (i = 0; i < file_count; i++) {
        entry = archive + EMXA_HEADER_SIZE + i * EMXA_ENTRY_SIZE;
        offset = read_le32(entry + EMXA_PATH_SIZE);
        file_size = read_le32(entry + EMXA_PATH_SIZE + 4);

        printk("- ");
        for (j = 0; j < EMXA_PATH_SIZE && entry[j] != '\0'; j++)
            printk("%c", entry[j]);
        printk(" offset=%x size=%x\n", offset, file_size);
    }
}
