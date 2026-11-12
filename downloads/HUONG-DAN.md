**AutoWheel — tự cuộn chuột cho Windows 10/11 64-bit.**

Đây là chương trình độc lập được viết mới. Nó chỉ gửi sự kiện bánh xe chuột qua API Windows `SendInput`. Không dùng lại EXE/DLL trong gói auto nhảy cũ; không mở tiến trình game, đọc/sửa bộ nhớ game, nạp DLL, cài driver, hook hay can thiệp Xigncode. Không có kết nối mạng, tải file, cơ chế cập nhật hoặc yêu cầu quyền Administrator.

Ứng dụng không biết lúc nào nhân vật chạm đất. Nó chỉ cuộn liên tục khi bạn giữ nút chuột. Để tạo thao tác nhảy, game cần có thiết lập cho phép gán nhảy vào cuộn lên hoặc cuộn xuống. Không có bảo đảm nhảy giống DLL cũ, tương thích với game, được nhà phát hành cho phép hoặc không bị anti-cheat nhận diện/chặn. Nếu game chặn đầu vào giả lập thì bộ cuộn này cũng có thể không hoạt động.

**Cách dùng:**

1. Đóng hoàn toàn bộ auto cũ, gồm loader và công cụ bypass đi kèm. Nếu chúng còn đang hoạt động hoặc game vẫn báo lỗi, khởi động lại máy trước khi thử ứng dụng mới để tách biệt nguyên nhân.
2. Trong phần thiết lập game, gán **nhảy** cho một hướng cuộn chuột nếu game hỗ trợ. Chọn cùng hướng trong AutoWheel. Mặc định ứng dụng là **cuộn xuống**.
3. Mở `AutoWheel.exe` bình thường. Không có yêu cầu chạy Administrator. Đặt độ trễ từ **1 đến 1000 ms**; mặc định **1 ms**. Chọn nút giữ: **chuột giữa**, **nút hông 1 (X1)** hoặc **nút hông 2 (X2)**.
4. Chuyển sang cửa sổ cần cuộn, đưa con trỏ vào bên trong và nhấn **F6**. Cửa sổ đang hoạt động lúc nhấn F6 được chọn làm đích.
5. **Thả rồi giữ** nút chuột đã chọn để cuộn. Thả nút để ngừng cuộn. Đây là chế độ giữ, không phải chế độ bật nhảy liên tục bằng một lần click như bản DLL cũ.
6. **F8** tắt bộ cuộn ngay. **F6** cũng bật/tắt. **F10** thoát hẳn ứng dụng.

Đổi hoặc đóng cửa sổ đích sẽ tự tắt. Sau đó cần nhấn F6 tại cửa sổ đích để bật lại. Đưa con trỏ ra ngoài cửa sổ sẽ tạm dừng; khi đưa vào lại phải thả rồi giữ nút một lần nữa. Chương trình cố kiểm tra cửa sổ và vị trí chuột trước khi gửi; Windows không cung cấp cơ chế gửi `SendInput` gắn cứng vào một HWND nên kiểm tra này là biện pháp hạn chế gửi nhầm, không phải bảo đảm tuyệt đối khi đổi cửa sổ đúng thời điểm gửi.

Ứng dụng không chặn nút chuột vật lý: nếu nút giữ đã được gán một hành động khác trong game, hành động đó vẫn có thể xảy ra. Các phím F6/F8/F10 được đăng ký làm phím tắt toàn cục trong lúc ứng dụng mở. Nếu phím đang được ứng dụng khác chiếm, AutoWheel sẽ báo và thoát.

**Độ trễ và giới hạn:** 1 ms là khoảng cách tối thiểu được yêu cầu giữa hai sự kiện, không phải cam kết 1000 lần mỗi giây. Nhịp thực tế phụ thuộc lịch chạy của Windows, tải CPU, chế độ tiết kiệm điện và cách game xử lý đầu vào. Mỗi lần gửi tương ứng một nấc cuộn chuẩn (`WHEEL_DELTA = 120`), không gửi bù hàng loạt khi hệ thống bị trễ. Chương trình dùng bộ định thời Windows, không vòng lặp bận chiếm CPU để ép tốc độ. Chỉ yêu cầu độ phân giải timer cao trong lúc bật và trả lại khi dừng/thoát.

Nếu không cuộn: kiểm tra F6 đã bật đúng cửa sổ, đã thả nút trước khi giữ lại, con trỏ nằm trong cửa sổ, hướng cuộn trùng với thiết lập game. Có thể thử với một trang văn bản dài trước: nhấn F6 trong cửa sổ đó rồi giữ nút. Thử nghiệm này chỉ kiểm tra cuộn chuột, không xác nhận game sẽ nhận nó. Nếu Windows trả lỗi `SendInput`, ứng dụng tự dừng; nếu Windows nhận sự kiện nhưng game bỏ qua, ứng dụng không thể phân biệt trường hợp đó với cuộn thành công trong game.

Nếu game báo `E019100B` hoặc một lỗi anti-cheat khác, dừng thử nghiệm và dùng kênh hỗ trợ chính thức. Không tắt bảo vệ, cài driver hoặc thay đổi quyền hệ thống để ép chạy. Đây không phải công cụ vượt phát hiện và không có chế độ ẩn mình. File mới chưa được ký số; nếu phần mềm bảo mật chặn, không cần tắt bảo vệ để chạy.

**Chẩn đoán ảnh của bản cũ:** `SharpMonoInjector` báo `Failed to enumerate process modules` / `Access is denied` khi tìm module Mono. Bước nạp `Jump.dll` chưa hoàn thành. Thông báo sau đó `Code: E019100B` ghi “Phát hiện chương trình lạ”. Kết hợp hai ảnh cho thấy cơ chế truy cập tiến trình cũ không hoạt động và hệ thống bảo vệ phát hiện một chương trình, nhưng chưa xác định chính xác EXE/DLL nào bị nhận diện. Không có bằng chứng cho thấy đổi delay hoặc đổi tên file sẽ sửa được lỗi này.

**Mã nguồn và kiểm thử:** bộ ZIP có source C, manifest, resource, script build và test. Kiểm thử state machine đã kiểm tra 12 tình huống về bật/tắt, mất focus, vị trí chuột, nhả/giữ nút và nhịp gửi, chạy với GCC/AddressSanitizer/UndefinedBehaviorSanitizer. EXE được biên dịch chéo và kiểm tra cấu trúc PE/import/manifest trên Linux. Chưa chạy giao diện trên Windows hoặc thử với Truy Kích 2/Xigncode; không có xác nhận hoạt động trong game hay không bị kick.

Để build lại với Python 3.12 và Zig 0.14.1:

```text
python -m pip install ziglang==0.14.1
python build.py
```

Kết quả nằm ở `build/AutoWheel.exe`. Không cần Python, Zig, .NET hoặc các DLL cũ để sử dụng EXE đã build; chỉ cần các thư viện hệ thống chuẩn Windows.

Để chạy test logic trên Linux với GCC:

```text
gcc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -g wheel_logic.c tests/test_wheel_logic.c -o build/test_wheel_logic
./build/test_wheel_logic
```
