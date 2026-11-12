**AutoWheel 1.1 — Home kích hoạt, chuột bật/tắt.** Dành cho Windows 10/11 64-bit.

Ứng dụng tự cuộn chuột qua Windows SendInput. Không dùng file cũ, không mở tiến trình game, đọc/sửa bộ nhớ, nạp DLL, cài driver, can thiệp anti-cheat hoặc yêu cầu Administrator. Không có kết nối mạng hay tự cập nhật. Ứng dụng không đọc trạng thái nhân vật chạm đất; để nhảy, game cần hỗ trợ gán thao tác nhảy vào cuộn lên hoặc cuộn xuống.

**Cách dùng:**

1. Đóng phiên bản AutoWheel/bộ auto cũ trước khi mở bản này, để tránh hai ứng dụng cùng gửi thao tác hoặc chiếm phím.
2. Trong phần thiết lập game, gán nhảy vào **cuộn xuống** (mặc định) hoặc **cuộn lên**, nếu game hỗ trợ. Chọn cùng hướng trong AutoWheel.
3. Mở `AutoWheel.exe` bình thường. Độ trễ mặc định **1 ms**, có thể chỉnh từ 1 đến 1000 ms.
4. Chuyển vào cửa sổ game, đưa con trỏ vào trong và nhấn **Home** để kích hoạt. Lúc này ứng dụng **chưa cuộn**.
5. **Bấm chuột giữa, nút hông 1 (X1) hoặc nút hông 2 (X2) một lần:** bắt đầu cuộn liên tục. Thả nút ra vẫn tiếp tục cuộn.
6. **Bấm một trong các nút đó lần nữa:** dừng cuộn, vẫn giữ trạng thái đã kích hoạt. Có thể bật bằng một nút rồi tắt bằng nút khác.
7. Đóng cửa sổ AutoWheel hoặc bấm **Thoát** để thoát hẳn. Nút **Hủy kích hoạt** trên ứng dụng cũng dừng hoàn toàn.

Mặc định cả ba nút chuột đều có thể bật/tắt. Có thể chọn chỉ chuột giữa, chỉ X1 hoặc chỉ X2 ở mục **Nút bật/tắt**. Hai nút macro hông phải được Windows nhận dạng là **X1/X2**; nếu phần mềm chuột đã đổi chúng thành tổ hợp phím khác thì ứng dụng không tự nhận dạng được các tổ hợp đó. Các nút macro bổ sung ngoài X1/X2 không được hỗ trợ trực tiếp.

Home là phím tắt bàn phím duy nhất được ứng dụng đăng ký. Nhấn Home lần nữa đặt lại chế độ đã kích hoạt nhưng đang dừng; sau đó bấm chuột để bắt đầu lại. Nếu đang giữ nút chuột khi nhấn Home, cần thả các nút ra trước rồi mới bấm. Giữ nút không khiến trạng thái bật/tắt lặp lại. Các nút được bấm cùng lúc chỉ được tính một lần bật/tắt trong cùng lượt đọc.

**Dừng khi đổi cửa sổ:** đổi hoặc đóng cửa sổ đích sẽ ngừng cuộn và hủy kích hoạt. Quay lại game cần nhấn Home rồi bấm chuột để chạy lại. Đưa con trỏ ra ngoài cửa sổ đích sẽ ngừng cuộn nhưng vẫn giữ kích hoạt; đưa chuột vào lại, thả các nút nếu đang giữ rồi bấm một lần để bắt đầu. Không tự tiếp tục chỉ vì con trỏ quay lại.

Ứng dụng kiểm tra cửa sổ và vị trí chuột trước khi gửi để hạn chế cuộn nhầm. Windows không cung cấp cơ chế gắn SendInput vào một HWND cố định nên việc kiểm tra không thể bảo đảm tuyệt đối khi đổi cửa sổ đúng lúc gửi. Nút chuột vật lý vẫn được chuyển tới ứng dụng/game như bình thường, nên hành động đã gán cho nút đó trong game vẫn có thể xảy ra.

**Độ trễ:** 1 ms là khoảng cách tối thiểu được yêu cầu giữa hai sự kiện, không phải bảo đảm 1000 lần/giây. Nhịp thực tế phụ thuộc Windows, CPU, tiết kiệm điện và cách game đọc đầu vào. Mỗi lần gửi là một nấc cuộn chuẩn (WHEEL_DELTA = 120). Không gửi bù dồn khi hệ thống bị trễ. Bộ định thời hoạt động trong lúc đã kích hoạt để theo dõi nút chuột, không dùng vòng lặp bận.

Nếu Home báo trùng phím, đóng phiên bản cũ hoặc ứng dụng khác đang đăng ký Home rồi mở lại. Nếu Home được nhấn khi chính cửa sổ AutoWheel đang được chọn, ứng dụng sẽ nhắc chuyển sang cửa sổ đích. Hướng cuộn, độ trễ và lựa chọn nút được áp dụng khi nhấn Home.

Có thể thử trên một văn bản dài có thanh cuộn trước khi thử trong game. Thử nghiệm đó chỉ xác nhận thao tác cuộn ở ứng dụng thường. Nếu Windows trả lỗi SendInput, ứng dụng tự dừng; nếu game bỏ qua sự kiện mà Windows đã nhận, ứng dụng không biết thao tác trong game có thành công hay không.

Không bảo đảm game chấp nhận thao tác, được nhà phát hành cho phép hoặc không bị anti-cheat chặn/kick. Đây không phải bản vượt Xigncode. Nếu xuất hiện lỗi anti-cheat, dừng thử và dùng kênh hỗ trợ chính thức. EXE chưa ký số; không cần tắt phần mềm bảo vệ để ép chạy.

**Kiểm thử:** đã kiểm thử logic bật/tắt bằng nhấn chuột, giữ/thả không lặp bật/tắt, phối hợp X1/X2/chuột giữa, dừng khi mất focus/ra ngoài cửa sổ và nhịp gửi bằng GCC cùng AddressSanitizer/UndefinedBehaviorSanitizer. EXE được biên dịch chéo và kiểm tra PE/import/manifest trên Linux. Chưa chạy giao diện trên Windows hoặc thử với Truy Kích 2/Xigncode.

Bộ ZIP chứa ứng dụng, mã nguồn C, manifest/resource, script build, test và mã băm SHA-256. Không chứa các DLL hoặc EXE từ bộ auto nhảy cũ.

Build lại với Python 3.12 và Zig 0.14.1:

```text
python -m pip install ziglang==0.14.1
python build.py
```

Kết quả: `build/AutoWheel.exe`. Để sử dụng EXE có sẵn, không cần Python, Zig hay .NET.

Chạy test logic trên Linux với GCC:

```text
gcc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -g wheel_logic.c tests/test_wheel_logic.c -o build/test_wheel_logic
./build/test_wheel_logic
```
