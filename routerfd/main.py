import openpyxl
from pathlib import Path

excel_list: list = []
output_can2fd = "generated_can2fd.txt"
output_fd2can = "generated_fd2can.txt"
code_can2fd_str: str = ""
code_fd2can_str: str = ""

def read_excel():
    global excel_list
    wb = openpyxl.load_workbook(filename = 'diff_table.xlsx')
    sheet1 = wb['Sheet1']
    first_flag = 0
    for row in sheet1.iter_rows():
        if first_flag == 0:
            first_flag += 1
            continue
        d = {
            "can_id": row[0].value,
            "can_cycle": row[3].value,
            "can_start": row[4].value,
            "can_length": row[5].value,
            "canfd_id": row[6].value,
            "canfd_cycle": row[9].value,
            "canfd_start": row[10].value,
            "canfd_length": row[11].value
        }
        excel_list.append(d)

def get_list_from_can_id(id: str) -> list:
    return list(filter(lambda x: x["can_id"] == id, excel_list))

def get_list_from_canfd_id(id: str) -> list:
    return list(filter(lambda x: x["canfd_id"] == id, excel_list))

def convert_matrix_to_bytes(li: list) -> list:
    l = 64
    b = int(l / 8)
    matrix = [0 for i in range(l)]
    byte_list = [0 for i in range(b)]

    for it in li:
        # 逆顺序的场合
        can_start = it["can_start"]
        can_length = it["can_length"]
        for i in range(can_start, can_start + can_length):
            index = i - 8 * (int(i / 8) - int(can_start / 8)) * 2
            matrix[index] = 1
            # if can_start % 8 + can_length <= 8:
            #     matrix[i] = 1
            # if 8 < can_start % 8 + can_length <= 16:
            #     if i % 8 + can_length <= 8:
            #         matrix[i] = 1
            #     elif i % 8 + can_length > 8:
            #         matrix[i - (i / 8 - can_start / 8) ]

        # 正顺序的场合
        # for i in range(it["can_start"], it["can_start"] + it["can_length"]):
        #     matrix[i] = 1

    for i in range(b):
        tmp_i = 0
        for j in range(8):
            tmp_i = tmp_i ^ (matrix[i * 8 + j] << j)
        byte_list[i] = tmp_i

    bytestr_list = ["{0:#0{1}x}".format(byte_list[i], 4) for i in range(b)]
    return bytestr_list

def main():
    global code_can2fd_str
    global code_fd2can_str
    read_excel()
    can_ids = set(filter(lambda x: x is not None, [it["can_id"] for it in excel_list]))
    for can_id in can_ids:
        li = get_list_from_can_id(can_id)
        canfd_id = li[0]["canfd_id"]
        # bytestr_list = convert_matrix_to_bytes(li)
        # code 头部
        # can2fd
        code_can2fd_str += f"""
case 0x{can_id}:
    for (int i = 0; i < 64; i++)
    {{
        RxMsg64.data8[i] = 0;
    }}
    RxMsg64.id = 0x{canfd_id};
    RxMsg64.msgtype = CAN_MSGTYPE_FDF;
    RxMsg64.dlc = CAN_LEN64_DLC;
"""
        # fd2can
        code_fd2can_str += f"""
case 0x{canfd_id}:
    for (int i = 0; i < 8; i++)
    {{
        RxMsg8.data8[i] = 0;
    }}
    RxMsg8.id = 0x{can_id};
    RxMsg8.msgtype = CAN_MSGTYPE_STANDARD;
    RxMsg8.dlc = CAN_LEN8_DLC;
"""
        # code 数据部，对同一ID中的每个字段进行操作
        for it in li:
            # 排除非对称字段
            if it["can_id"] is None or it["canfd_id"] is None:
                continue
            # CAN 报文矩阵生成
            l = 64
            b = int(l / 8)
            matrix = [0 for i in range(l)]
            byte_list = [0 for i in range(b)]
            can_start = it["can_start"]
            can_length = it["can_length"]
            canfd_start = it["canfd_start"]
            canfd_length = it["canfd_length"]
            for i in range(can_start, can_start + can_length):
                index = i - 8 * (int(i / 8) - int(can_start / 8)) * 2
                matrix[index] = 1
            for i in range(b):
                tmp_i = 0
                for j in range(8):
                    tmp_i = tmp_i ^ (matrix[i * 8 + j] << j)
                byte_list[i] = tmp_i

            bytestr_list = ["{0:#0{1}x}".format(byte_list[i], 4) for i in range(b)]

            # 需要扩展时的循环次数，因边界条件因素需要+1
            loop_times = int((can_start % 8 + can_length - 1) / 8) + 1
            for i in range(loop_times):
                data_index = int(can_start / 8)
                datafd_index = int(canfd_start / 8)
                code_can2fd_str += f"""
    RxMsg64.data8[{datafd_index - i}] |= RxMsg.data8[{data_index - i}] & {bytestr_list[data_index - i]};"""

        # code 尾部
        code_can2fd_str += f"""

    CAN_Write (CAN_BUS2, &RxMsg64);
    break;"""
            
    Path(output_can2fd).write_text(code_can2fd_str)

main()