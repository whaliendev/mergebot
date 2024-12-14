package space.whalien.conflictmanager.services;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.data.redis.core.RedisTemplate;
import org.springframework.lang.Nullable;
import org.springframework.stereotype.Service;

import java.util.HashMap;
import java.util.Map;

@Service
public class RedisService {

    private static final Logger logger = LoggerFactory.getLogger(RedisService.class);

    private final RedisTemplate<String, String> redisTemplate;

    public RedisService(RedisTemplate<String, String> redisTemplate) {
        this.redisTemplate = redisTemplate;
    }

    public boolean setString(String key, String value) {
        try {
            logger.debug("Set string value for key: {}", key);
            redisTemplate.opsForValue().set(key, value);
            return true;
        } catch (Exception e) {
            logger.error("Failed to set string value for key: {}", key, e);
            return false;
        }
    }

    public @Nullable String getString(String key) {
        try {
            logger.debug("Get string value for key: {}", key);
            return redisTemplate.opsForValue().get(key);
        } catch (Exception e) {
            logger.error("Failed to get string value for key: {}", key, e);
            return null;
        }
    }

    public boolean setIntHash(String key, String hashKey, String value) {
        try {
            logger.debug("Set hash value for key: {}", key);
            redisTemplate.opsForHash().put(key, hashKey, value);
            return true;
        } catch (Exception e) {
            logger.error("Failed to set hash value for key: {}", key, e);
            return false;
        }
    }

    public @Nullable Map<Integer, Object> getIntHash(String key) {
        try {
            logger.debug("Get hash value for key: {}", key);
            Map<Object, Object> value = redisTemplate.opsForHash().entries(key);
            Map<Integer, Object> result = new HashMap<>();
            for (Map.Entry<Object, Object> entry : value.entrySet()) {
                result.put(Integer.valueOf((String) entry.getKey(), 10), entry.getValue());
            }
            return result;
        } catch (Exception e) {
            logger.error("Failed to get hash value for key: {}", key, e);
            return null;
        }
    }

    public boolean deleteHash(String key) {
        // delete hash key
        try {
            redisTemplate.opsForHash().getOperations().delete(key);
            logger.debug("Delete hash key: {}", key);
            return true;
        } catch (Exception e) {
            logger.error("Failed to delete hash key: {}", key, e);
            return false;
        }
    }

    public boolean deleteKey(String key) {
        try {
            redisTemplate.opsForValue().getOperations().delete(key);
            logger.debug("Delete key: {}", key);
            return true;
        } catch (Exception e) {
            logger.error("Failed to delete key: {}", key, e);
            return false;
        }
    }

}


