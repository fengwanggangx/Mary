function main(nType, param)
    if nType == 1 then
        -- 检查param有效性
        if param == nil then
            CPrint("错误：param不能为空")
            return -1
        end
        local result = 100  -- 业务处理结果
        local n = CExecute(999)
        local n2 = CExecute(998)
        param:SetInt(7895)  -- 修正语法：使用冒号调用
        CPrint("XXXXX")
        return n  -- 返回第一个CExecute的结果
    elseif nType == 2 then
        -- 处理类型2的业务
        if param ~= nil then
            return 200  -- 成功
        else
            return -200  -- 参数错误
        end
    else
        return -100  -- 未知类型错误
    end
end