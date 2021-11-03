
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
            "can_signal": row[1].value,
            "can_cycle": row[3].value,
            "can_start": row[4].value,
            "can_length": row[5].value,
            "canfd_id": row[6].value,
            "canfd_signal": row[7].value,
            "canfd_cycle": row[9].value,
            "canfd_start": row[10].value,
            "canfd_length": row[11].value,
            "sender": row[12].value
        }
        excel_list.append(d)

def get_list_from_can_id(id: str) -> list:
    return list(filter(lambda x: x["can_id"] == id, excel_list))

def get_list_from_canfd_id(id: str) -> list:
    return list(filter(lambda x: x["canfd_id"] == id, excel_list))

def convert_matrix_to_bytes(it: dict, _type: str) -> list:
    can_start = it["can_start"]
    can_length = it["can_length"]
    canfd_start = it["canfd_start"]
    canfd_length = it["canfd_length"]

    if _type == "can":
        l = 8 * 8
        b = int(l / 8)
        start = can_start
        length = can_length
    else:
        l = 8 * 64
        b = int(l / 8)
        start = canfd_start
        length = canfd_length
    matrix = [0 for i in range(l)]
    byte_list = [0 for i in range(b)]
    
    for i in range(start, start + length):
        index = i - 8 * (int(i / 8) - int(start / 8)) * 2
        matrix[index] = 1
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
        if li[0]["sender"] == "EPS": 
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
        else:
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
            bytestr_can_list = convert_matrix_to_bytes(it, "can")
            # CANFD 报文矩阵生成
            bytestr_canfd_list = convert_matrix_to_bytes(it, "canfd")

            can_start = it["can_start"]
            can_length = it["can_length"]
            canfd_start = it["canfd_start"]
            canfd_length = it["canfd_length"]

            # 需要扩展时的循环次数，因边界条件因素需要+1

            # can 2 canfd 报文代码生成
            if it["sender"] == "EPS":
                loop_times = int((can_start % 8 + can_length - 1) / 8) + 1
                for i in range(loop_times):
                    data_index = int(can_start / 8)
                    datafd_index = int(canfd_start / 8)
                    # === can 2 canfd 自定义特殊情况段 开始 ===
                    
                    # 助力转向故障位：信号长度不同，但可以直接位移
                    if it["can_signal"] in ["EpsFaild"]:
                        shift = can_start % 8 - canfd_start % 8
                        code_can2fd_str += f"""
    RxMsg64.data8[{datafd_index - i}] |= (RxMsg.data8[{data_index - i}] & {bytestr_can_list[data_index - i]}) >> {shift};"""

                    # 驾驶员手力矩：信号长度不同，11->12，需要单独处理
                    if it["can_signal"] in ["EPS_MeasuredTorsionBarTorque"]:
                        shift = can_start % 8 - canfd_start % 8 
                        if data_index - i == 1:
                            code_can2fd_str += f"""
    RxMsg64.data8[{datafd_index - i}] |= ((RxMsg.data8[{data_index - i}] & {bytestr_can_list[data_index - i]}) >> {shift}) | RxMsg.data8[{data_index - i - 1}] << 7;"""
                        if data_index - i == 0:
                            code_can2fd_str += f"""
    RxMsg64.data8[{datafd_index - i}] |= (RxMsg.data8[{data_index - i}] & {bytestr_can_list[data_index - i]}) >> {shift};"""

                    # CRC 校验部分，先全部置零，或直接continue跳过即可
                    if it["can_signal"] in ["EPS_CRCCheck_170", "EpsCrcChk180", "EPS_CRCCheck_17E", "EPS_CRCCheck_24F"]:
                        continue

                    # === can 2 canfd 自定义特殊情况段 结束 ===
                    code_can2fd_str += f"""
    RxMsg64.data8[{datafd_index - i}] |= RxMsg.data8[{data_index - i}] & {bytestr_can_list[data_index - i]};"""

            # canfd 2 can 报文代码生成
            else:
                loop_times = int((canfd_start % 8 + canfd_length - 1) / 8) + 1
                for i in range(loop_times):
                    data_index = int(can_start / 8)
                    datafd_index = int(canfd_start / 8)
                    # === canfd 2 can 自定义特殊情况段 开始 ===

                    if it["canfd_signal"] in ["PwrTrainSts", "CdcSteerModSet"]:
                        shift = canfd_start % 8 - can_start % 8
                        code_fd2can_str += f"""
    RxMsg8.data8[{data_index - i}] |= (RxMsg.data8[{datafd_index - i}] & {bytestr_canfd_list[datafd_index - i]}) >> {shift};"""

                    # CRC 校验部分，先全部置零，或直接continue跳过即可
                    if it["canfd_signal"] in ["APA_CRCCheck_247", "ESP_CRCCheck_17A", "VcuCrcChk1D1", "ACC_CRCCheck_1BA_0", "IbcuCrcChk20B"]:
                        continue

                    # === canfd 2 can 自定义特殊情况段 结束 ===
                    code_fd2can_str += f"""
    RxMsg8.data8[{data_index - i}] |= RxMsg.data8[{datafd_index - i}] & {bytestr_canfd_list[datafd_index - i]};"""

        # code 尾部
        # can 2 canfd 尾部
        if li[0]["sender"] == "EPS": 

            # CRC 校验覆写部分
            if can_id in ["170", "180", "17E", "24F"]:
                code_fd2can_str += f"""
    RxMsg64.data8[7] = CRCSAECalc(RxMsg64.data8, 7);"""

            # 传入 BUS2
            code_can2fd_str += f"""

    CAN_Write (CAN_BUS2, &RxMsg64);
    break;"""
        # canfd 2 can 尾部
        else: 
            # CRC 校验覆写部分
            if canfd_id in ["247", "17A", "17D", "1BA", "20B"]:
                code_fd2can_str += f"""
    RxMsg8.data8[7] = CRCSAECalc(RxMsg8.data8, 7);"""

            # 传入 BUS1
            code_fd2can_str += f"""

    CAN_Write (CAN_BUS1, &RxMsg8);
    break;"""
            
    Path(output_can2fd).write_text(code_can2fd_str)
    Path(output_fd2can).write_text(code_fd2can_str)

main()